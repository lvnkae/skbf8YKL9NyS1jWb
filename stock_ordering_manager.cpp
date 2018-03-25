/*!
 *  @file   stock_ordering_manager.cpp
 *  @brief  �������Ǘ���
 *  @date   2017/12/26
 */
#include "stock_ordering_manager.h"

#include "securities_session.h"
#include "stock_holdings_keeper.h"
#include "stock_holdings.h"
#include "stock_portfolio.h"
#include "stock_trading_command_fwd.h"
#include "stock_trading_command.h"
#include "stock_trading_tactics.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"
#include "trade_utility.h"
#include "update_message.h"

#include "cipher_aes.h"
#include "garnet_time.h"
#include "yymmdd.h"
#include "twitter/twitter_session.h"
#include "utility/utility_datetime.h"
#include "utility/utility_string.h"

#include <list>
#include <thread>

namespace trading
{

namespace
{
void AddErrorMsg(const std::wstring& err, std::wstring& dst)
{
    if (dst.empty()) {
        dst = err;
    } else {
        dst += garnet::twitter::GetNewlineString() + err;
    }
}
std::wstring GetErrorMsgHeader()
{
    const std::wstring nl(std::move(garnet::twitter::GetNewlineString()));
    return nl + L"[error]" + nl;
}
} // namespace

class StockOrderingManager::PIMPL
{
private:
    /*!
     *  @brief  �헪���ʏ��
     *  @note   a��b����v����Γ����Ac����v�����瓯��̒���
     */
    struct TacticsIdentifier
    {
        int32_t m_tactics_id;   //!< �헪ID...(a)
        int32_t m_group_id;     //!< �헪�O���[�vID...(b)
        int32_t m_unique_id;    //!< �헪�����ŗLID...(c)

        TacticsIdentifier(const StockTradingCommand& command)
        : m_tactics_id(command.GetTacticsID())
        , m_group_id(command.GetOrderGroupID())
        , m_unique_id(command.GetOrderUniqueID())
        {
        }
    };

    /*!
     *  @brief  �ً}���[�h���
     */
    struct EmergencyModeState
    {
        uint32_t m_code;
        int32_t m_tactics_id;
        std::unordered_set<int32_t> m_group;

        int64_t m_timer; //! �c�莞��

        EmergencyModeState(uint32_t code,
                           int32_t tactics_id,
                           const std::unordered_set<int32_t>& group,
                           int64_t timer)
        : m_code(code)
        , m_tactics_id(tactics_id)
        , m_group(group)
        , m_timer(timer)
        {
        }

        void AddGroupID(const std::unordered_set<int32_t>& group)
        {
            for (int32_t group_id: group) {
                m_group.insert(group_id);
            }
        }
    };

    //! �،���ЂƂ̃Z�b�V����
    SecuritiesSessionPtr m_pSecSession;
    //! twitter�Ƃ̃Z�b�V����
    garnet::TwitterSessionForAuthorPtr m_pTwSession;

    //! ����헪�f�[�^<�헪ID, �헪�f�[�^>
    std::unordered_map<int32_t, StockTradingTactics> m_tactics;
    //! �헪�f�[�^�R�t�����<�����R�[�h, �헪ID>
    std::vector<std::pair<uint32_t, int32_t>> m_tactics_link;
    //! �ً}���[�h����[�~���b] ���O���ݒ肩��擾
    const int64_t m_emergency_time_ms;
    //! �Ď�����
    StockBrandContainer m_monitoring_brand;
    //! �Ď������f�[�^<��������, <�����R�[�h, 1�������̉��i�f�[�^>>
    std::unordered_map<eStockInvestmentsType, std::unordered_map<uint32_t, StockValueData>> m_monitoring_data;
    //! �ۗL�����Ǘ�
    StockHoldingsKeeper m_holdings;

    //! ������M�܂Ŕ����������b�N����
    bool m_b_lock_odmng_and_wait_execinfo;

    //! ���߃��X�g
    std::list<StockTradingCommandPtr> m_command_list;
    //! �ً}���[�h���
    std::list<EmergencyModeState> m_emergency_state;
    //! ���ʑ҂����� ���v�f����1��0/������1����������
    std::vector<StockTradingCommandPtr> m_wait_order;
    //! �����ςݒ���<��������, <�����ԍ�(�Ǘ��p), ����>>
    std::unordered_map<eStockInvestmentsType, std::unordered_map<int32_t, StockTradingCommandPtr>> m_server_order;
    //! ������蒍��<�����R�[�h, <��蒍������>> ��������/�V�K�M�p��/�V�K�M�p�����ΏہA�Ĕ�������Ɏg��
    typedef std::pair<TacticsIdentifier, std::vector<int32_t>> StockExecOrderIdentifier; // ��蒍������<���ʏ��, ���ʌŗLID�Q>
    std::unordered_map<uint32_t, std::list<StockExecOrderIdentifier>> m_exec_order;
    //! �����ԍ��Ή��\<�\���p, �Ǘ��p>
    std::unordered_map<int32_t, int32_t> m_server_order_id;

    //! ���݂̑Ώێ����
    eStockInvestmentsType m_investments;
    //! �o�ߎ���[�~���b]
    int64_t m_tick_count;
    //! �ŏI������������(tick)
    int64_t m_last_tick_rcv_rep_order;

    /*!
     *  @brief  ������胁�b�Z�[�W�o��
     *  @param  command_ptr ��������
     *  @param  exec_info   �����(1������)
     *  @param  date        ��������
     *  @note   �o�͐��twitter
     */
    void OutputExecMessage(const StockTradingCommandPtr& command_ptr,
                           const StockExecInfoAtOrder& ex_info)
    {
        const int32_t order_id = ex_info.m_user_order_id;
        const StockOrder order(std::move(command_ptr->GetOrder()));
        const std::wstring name(m_monitoring_brand[order.GetCode()]);
        const std::wstring zone(L"JST");
        for (const auto& ex: ex_info.m_exec) {
            std::wstring src(L"���");
            order.BuildMessageString(order_id, name, ex.m_number, ex.m_value, src);
            const std::wstring date(
                std::move(garnet::utility_datetime::ToRFC1123(ex.m_date, ex.m_time, zone)));
            m_pTwSession->Tweet(date, src);
        }
    }

    /*!
     *  @brief  �����R�[���o�b�N
     *  @param  b_result    ����
     *  @param  rcv_order   ��������
     *  @param  sv_date     �T�[�o����
     *  @param  investments �������̎�������
     *  @note   �ʐM�x���Ōׂ��\��������̂Ō���(this)��investments�͎g��Ȃ�
     */
    void StockOrderCallback(bool b_result,
                            const RcvResponseStockOrder& rcv_order,
                            const std::wstring& sv_date,
                            eStockInvestmentsType investments)
    {
        std::wstring message((b_result) ?L"������t" : L"�������s");
        // ������������(tick)�X�V
        m_last_tick_rcv_rep_order = garnet::utility_datetime::GetTickCountGeneral();
        //
        if (m_wait_order.empty()) {
            // �Ȃ��������҂����Ȃ�(error)
            message += GetErrorMsgHeader() + L"%wait_order is empty";
            m_pTwSession->Tweet(sv_date, message);
        } else {
            const StockTradingCommandPtr& w_cmd_ptr = m_wait_order.front();
            const StockOrder w_order(std::move(w_cmd_ptr->GetOrder()));
            //
            std::wstring err_msg;
            if (b_result) {
                if (w_order != rcv_order) {
                    // ��t�Ƒ҂����H������Ă�(error)
                    err_msg = L"isn't equal %wait_order and %rcv_order";
                }
                switch (w_order.m_type)
                {
                case ORDER_BUY:
                case ORDER_SELL:
                case ORDER_REPSELL:
                case ORDER_REPBUY:
                    m_server_order[investments].emplace(rcv_order.m_order_id, w_cmd_ptr);
                    m_server_order_id.emplace(rcv_order.m_user_order_id, rcv_order.m_order_id);
                    break;
                case ORDER_CORRECT:
                    {
                        const auto itID = m_server_order_id.find(rcv_order.m_user_order_id);
                        if (itID != m_server_order_id.end()) {
                            auto& sv_order(m_server_order[investments]);
                            auto itSvOrder = sv_order.find(itID->second);
                            if (itSvOrder == sv_order.end()) {
                                AddErrorMsg(L"not found %server_order", err_msg);
                            } else {
                                // ���i�㏑��
                                itSvOrder->second->SetOrderValue(w_order.m_value);
                            }
                        } else {
                            AddErrorMsg(L"fail to cnv %order_id from %user_order_id", err_msg);
                        }
                    }
                    break;
                case ORDER_CANCEL:
                    {
                        const auto itID = m_server_order_id.find(rcv_order.m_user_order_id);
                        if (itID != m_server_order_id.end()) {
                            // �폜
                            if (0 == m_server_order[investments].erase(itID->second)) {
                                AddErrorMsg(L"fail to erase %server_order", err_msg);
                            }
                        } else {
                            AddErrorMsg(L"fail to cnv %order_id from %user_order_id", err_msg);
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            // �ʒm
            {
                if (w_order.m_type == ORDER_NONE) {
                    // order����ŌĂ΂�邱�Ƃ�����(�������s��)
                } else {
                    const std::wstring name(m_monitoring_brand[w_order.GetCode()]);
                    w_order.BuildMessageString(rcv_order.m_user_order_id,
                                               name, w_order.m_number, w_order.m_value,
                                               message);
                }
                if (!err_msg.empty()) {
                    message += GetErrorMsgHeader() + err_msg;
                }
                m_pTwSession->Tweet(sv_date, message);
            }
            // �����҂�����(���ۖ��Ȃ�)
            m_wait_order.pop_back();
            // ���s���Ă��玟�̖����擾�܂Ŕ������������b�N����
            if (!b_result) {
                m_b_lock_odmng_and_wait_execinfo = true;
            }
        }
    }

    /*!
     *  @brief  �ً}���[�h��ԓo�^
     *  @param  command     ����
     */
    void EntryEmergencyState(const StockTradingCommand& command)
    {
        const uint32_t code = command.GetCode();
        const int32_t tactics_id = command.GetTacticsID();
        const std::unordered_set<int32_t> em_group(std::move(command.GetEmergencyTargetGroup()));
        for (auto& emstat: m_emergency_state) {
            if (emstat.m_code == code && emstat.m_tactics_id == tactics_id) {
                // ���łɂ���΍X�V
                emstat.AddGroupID(em_group);
                emstat.m_timer = m_emergency_time_ms;
                return;
            }
        }
        m_emergency_state.emplace_back(code, tactics_id, em_group, m_emergency_time_ms);
    }

    /*!
     *  @brief  �������
     *  @param  command     �������
     *  @param  investments ��������
     */
    void CancelOrderCommand(const StockTradingCommand& command, eStockInvestmentsType investments)
    {
        const std::unordered_set<int32_t> em_group(std::move(command.GetEmergencyTargetGroup()));
        const int32_t tactics_id = command.GetTacticsID();
        const uint32_t code = command.GetCode();

        // �����O�������
        {
            auto itRmv = std::remove_if(m_command_list.begin(),
                                        m_command_list.end(),
                                        [&command, &em_group](const StockTradingCommandPtr& dst)
            {
                const StockTradingCommand& dst_command(*dst);
                if (!dst_command.IsOrder()) {
                    return false; // �������߂���Ȃ�
                }
                if (command.GetCode() != dst_command.GetCode() ||
                    command.GetTacticsID() != dst_command.GetTacticsID()) {
                    return false; // �ʖ����܂��͕ʐ헪
                }
                if (ORDER_CANCEL == dst_command.GetOrderType()) {
                    return false; // ������߂͎������Ȃ�
                }
                const int32_t group_id = dst_command.GetOrderGroupID();
                for (const int32_t em_gid: em_group) {
                    if (group_id == em_gid) {
                        return true;
                    }
                }
                return false;
            });
            if (m_command_list.end() != itRmv) {
                m_command_list.erase(itRmv, m_command_list.end());
            }
        }

        // �����ςݒ������
        for (const auto& sv_order: m_server_order[investments]) {
            const StockTradingCommand& sv_cmd(*sv_order.second);
            const int32_t sv_tactics_id = sv_cmd.GetTacticsID();
            if (sv_tactics_id == tactics_id && sv_cmd.GetCode() == code) {
                const int32_t sv_order_id = sv_order.first;
                auto itCr = std::find_if(m_command_list.begin(),
                                            m_command_list.end(),
                                            [sv_order_id](const StockTradingCommandPtr& dst) {
                    return dst->GetOrderType() == ORDER_CANCEL &&
                            dst->GetOrderID() == sv_order_id;
                });
                if (itCr != m_command_list.end()) {
                    continue; // �����ς�ł���
                }
                const int32_t sv_group_id = sv_cmd.GetOrderGroupID();
                auto itEm = std::find(em_group.begin(), em_group.end(), sv_group_id);
                if (itEm != em_group.end()) {
                    // ���������擪�ɐς�
                    m_command_list.emplace_front(
                        new StockTradingCommand_ControllOrder(sv_cmd,
                                                              ORDER_CANCEL,
                                                              sv_order_id));
                }
            }
        }
    }

    /*!
     *  @brief  �����ςݒ����`�F�b�N
     *  @param  command     �o�����Ƃ��Ă閽��
     *  @param  investments ��������
     *  @note   ������������������true��Ԃ�
     *  @note     ���� -> �e��
     *  @note     ���� -> command������ -> �e��
     *  @noteq         -> command����� -> ���i����
     */
    bool CheckServerOrder(const StockTradingCommand& command,
                          eStockInvestmentsType investments)
    {
        const int32_t tactics_id = command.GetTacticsID();
        const uint32_t code = command.GetCode();
        const int32_t tac_uqid = command.GetOrderUniqueID();

        bool b_command_reject = false;

        // �����ςݒ����`�F�b�N
        for (const auto& sv_order: m_server_order[investments]) {
            const StockTradingCommand& sv_cmd(*sv_order.second);
            if (command.IsSameAttrOrder(sv_cmd)) {
                b_command_reject = true;
                if (sv_cmd.GetOrderUniqueID() >= tac_uqid) {
                    // ����A�܂��͓�����ʒ��������ς݂Ȃ̂Ŗ���
                    continue;
                }
                // �������ʖ��ߔ����ς݂̂��߉��i�����𖖔��ɐς�
                m_command_list.emplace_back(
                    new StockTradingCommand_ControllOrder(command,
                                                          ORDER_CORRECT,
                                                          sv_order.first));
            }
        }
        // �����ҋ@�����`�F�b�N
        for (auto& lcommand: m_command_list) {
            if (lcommand->IsSameBuySellOrder(command)) {
                b_command_reject = true;
                if (command.GetOrderUniqueID() > lcommand->GetOrderUniqueID()) {
                    // �㏑��(�㏟��)
                    lcommand->CopyBuySellOrder(command);
                }
            }
        }

        return b_command_reject;
    }

    /*!
     *  @brief  ������ߓo�^
     *  @param  command     ����
     *  @param  investments ��������
     */
    void EntryCommand(const StockTradingCommandPtr& command_ptr, 
                      eStockInvestmentsType investments)
    {
        if (m_b_lock_odmng_and_wait_execinfo) {
            return;
        }

        StockTradingCommand& command(*command_ptr);
        const int32_t tactics_id = command.GetTacticsID();
        const uint32_t code = command.GetCode();

        // ���ߎ�ʂ��Ƃ̏���
        switch (command.GetType())
        {
        case StockTradingCommand::EMERGENCY:
            // �ً}���[�h��ԓo�^
            EntryEmergencyState(command);
            // �������(���Ŗ��ߐς�ł�)
            CancelOrderCommand(command, investments);
            break;

        case StockTradingCommand::REPAYMENT_LEV_ORDER:
        case StockTradingCommand::BUYSELL_ORDER:
            {
                // �������ʑ҂������`�F�b�N
                if (!m_wait_order.empty()) {
                    const StockTradingCommand& w_cmd = *m_wait_order.front();
                    if (command.IsSameAttrOrder(*m_wait_order.front())) {
                        // �����������҂����Ă�̂Œe��(����������ɑΏ�����)
                        return;
                    }
                }
                // �����ςݒ����`�F�b�N(&���i����)
                if (CheckServerOrder(command, investments)) {
                    return; // ��s�����ɔs���������������̂őł��؂�
                }

                const eOrderType odtype = command.GetOrderType();
                const bool b_leverage = command.IsLeverageOrder();

                // ������蒍���`�F�b�N(�V�K���������̂�)
                if (odtype == ORDER_BUY || (odtype == ORDER_SELL && b_leverage)) {
                    for (const auto& ex_order: m_exec_order[code]) {
                        const TacticsIdentifier& tc_id(ex_order.first);
                        if (command.GetTacticsID() != tc_id.m_tactics_id ||
                            command.GetOrderGroupID() != tc_id.m_group_id) {
                            continue;
                        }
                        if (!b_leverage) {
                            // �����͍Ē����s��(�������ς��ʓ|�������̂�)
                            return;
                        }
                        if (m_holdings.CheckPosition(code, ex_order.second)) {
                            // �O�񒍕����̌��ʂ��c���Ă�
                            return;
                        }
                    }
                }

                // �ۗL�����`�F�b�N�E�S���w��W�J�E�������w��W�J
                if (odtype == ORDER_REPBUY || odtype == ORDER_REPSELL) {
                    // �M�p�ԍϔ���
                    const bool b_sell = (odtype == ORDER_REPBUY); // �Ԕ��Ȃ�Δ����ʂ𒲂ׂ�
                    const garnet::YYMMDD bg_date(std::move(command.GetRepLevBargainDate()));
                    if (!bg_date.empty()) {
                        // ���ʎw��ԍ�
                        const float64 bg_value = command.GetRepLevBargainValue();
                        const int32_t have_num
                            = m_holdings.GetPositionNumber(code, bg_date, bg_value, b_sell);
                        if (have_num <= 0) {
                            return; // �ۗL���ĂȂ�
                        }
                        const int32_t req_num = command.GetOrderNumber();
                        if (req_num < 0) {
                            // �S���w��
                            command.SetOrderNumber(have_num);
                        } else if (have_num < req_num) {
                            return; // ����Ȃ�
                        }
                    } else {
                        // �����w��ԍ�
                        bool b_add_command = false;
                        int32_t req_num = command.GetOrderNumber();
                        if (!m_holdings.CheckPosition(code, b_sell, req_num)) {
                            return; // ����Ȃ�
                        }
                        const auto pos_list(std::move(m_holdings.GetPosition(code, b_sell)));
                        if (pos_list.empty()) {
                            return; // ����Ă邱�Ƃ��m�F������Ȃ̂Ɏ擾�ł��Ȃ�(error)
                        }
                        const StockOrder order(std::move(command.GetOrder()));
                        for (const auto& pos: pos_list) {
                            int32_t order_num = 0;
                            if (req_num < 0) {
                                // �S���w��
                                order_num = pos.m_number;
                            } else if (req_num > pos.m_number) {
                                // �v���������ʂ�葽��
                                order_num = pos.m_number;
                                req_num -= pos.m_number;
                            } else {
                                // �v������d��������
                                order_num = req_num;
                                req_num = 0;
                            }
                            if (!b_add_command) {
                                // ����͍��̖��߂��g�p
                                command.SetOrderNumber(order_num);
                                command.SetRepLevBargain(pos.m_date, pos.m_value);
                                b_add_command = true;
                            } else {
                                // ���ʂ��ׂ����͒ǉ��R�}���h(�擪�ɐς�)
                                m_command_list.emplace_front(
                                    new StockTradingCommand_RepLevOrder(investments,
                                                                        code,
                                                                        tactics_id,
                                                                        command.GetOrderGroupID(),
                                                                        command.GetOrderUniqueID(),
                                                                        odtype,
                                                                        order.m_condition,
                                                                        order_num,
                                                                        order.m_value,
                                                                        pos.m_date,
                                                                        pos.m_value));
                            }
                            if (req_num == 0) {
                                break;
                            }
                        }
                    }
                } else if (odtype == ORDER_SELL && !b_leverage) {
                    // ������
                    const int32_t have_num = m_holdings.GetSpotTradingStockNumber(code);
                    if (have_num <= 0) {
                        return; // �ۗL���ĂȂ�
                    }
                    const int32_t req_num = command.GetOrderNumber();
                    if (req_num < 0) {
                        // �S���w��
                        command.SetOrderNumber(have_num);
                    } else if (have_num < req_num) {
                        return; // ����Ȃ�
                    }
                }
            }
            // �����ɐς�
            m_command_list.emplace_back(command_ptr);
            break;

        default:
            break;
        }
    }

    /*!
     *  @brief  �헪����
     *  @param  investments ���ݎ�������
     *  @param  now_time    ���ݎ����b
     *  @param  sec_time    ���Z�N�V�����J�n����
     *  @param  valuedata   ���i�f�[�^(1�������)
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     */
    void InterpretTactics(eStockInvestmentsType investments,
                          const garnet::HHMMSS& now_time,
                          const garnet::HHMMSS& sec_time,
                          const std::unordered_map<uint32_t, StockValueData>& valuedata,
                          TradeAssistantSetting& script_mng)
    {
        std::unordered_set<int32_t> blank_group;
        for (const auto& link: m_tactics_link) {
            const uint32_t code = link.first;
            const int32_t tactics_id = link.second;
            //
            const auto itVData = valuedata.find(code);
            if (itVData == valuedata.end()) {
                continue; // ���i�f�[�^���܂��Ȃ�
            }
            //
            const auto itEmStat
                = std::find_if(m_emergency_state.begin(),
                               m_emergency_state.end(),
                               [code, tactics_id](const EmergencyModeState& emstat)
            {
                return emstat.m_code == code && emstat.m_tactics_id == tactics_id;
            });
            //
            const auto& r_group =
                (itEmStat != m_emergency_state.end()) ?itEmStat->m_group 
                                                      :blank_group;
            auto tactics(m_tactics[tactics_id]);
            tactics.Interpret(investments, now_time, sec_time,
                              r_group, itVData->second,
                              script_mng,
                              [this, investments](const StockTradingCommandPtr& command_ptr)
            {
                EntryCommand(command_ptr, investments);
            });
        }
    }

    /*!
     *  @brief  �C�ӂ̖��߂���������
     *  @param  command     ����
     *  @param  investments ��������
     *  @param  aes_pwd
     *  @param  tickCount   �o�ߎ���[�~���b]
     */
    bool IssueOrderCore(const StockTradingCommand& command,
                        eStockInvestmentsType investments,
                        const garnet::CipherAES_string& aes_pwd,
                        int64_t tickCount)
    {
        const auto callback = [this, investments](bool b_result,
                                                  const RcvResponseStockOrder& rcv_order,
                                                  const std::wstring& sv_date) {
            StockOrderCallback(b_result, rcv_order, sv_date, investments);
        };

        if (!command.IsOrder()) {
            // �������߂���Ȃ�(error)
            return false;
        }
        if (m_b_lock_odmng_and_wait_execinfo) {
            // �������擾�܂Ń��b�N
            return false;
        }
        const int64_t MIN_ORDER_INTV_TICK = garnet::utility_datetime::ToMiliSecondsFromSecond(1);
        if (tickCount < m_last_tick_rcv_rep_order + MIN_ORDER_INTV_TICK) {
            // �A�ˋ֎~
            return false;
        }

        const StockOrder order(std::move(command.GetOrder()));
        const StockCode& s_code(order.RefCode());

        std::wstring pwd;
        if (!aes_pwd.Decrypt(pwd)) {
            // �������s
            return false;
        }

        switch (order.m_type)
        {
        case ORDER_BUY:
            m_pSecSession->BuySellOrder(order, pwd, callback);
            break;
        case ORDER_SELL:
            if (!m_holdings.CheckSpotTradingStock(s_code, order.GetNumber())) {
                // ���s��(���O�ŋN���蓾��)
                return false;
            }
            m_pSecSession->BuySellOrder(order, pwd, callback);
            break;
        case ORDER_CORRECT:
            m_pSecSession->CorrectOrder(command.GetOrderID(), order, pwd, callback);
            break;
        case ORDER_CANCEL:
            m_pSecSession->CancelOrder(command.GetOrderID(), pwd, callback);
            break;
        case ORDER_REPSELL:
        case ORDER_REPBUY:
            {
                const garnet::YYMMDD bg_date(std::move(command.GetRepLevBargainDate()));
                const float64 bg_value = command.GetRepLevBargainValue();
                const bool b_sell = order.m_type == ORDER_REPBUY; // �Ԕ��Ȃ�Δ����ʂ𒲂ׂ�
                if (!m_holdings.CheckPosition(s_code,
                                              bg_date,
                                              bg_value,
                                              b_sell,
                                              order.GetNumber())) {
                    // ���s��(���O�ŋN���蓾��)
                    return false;
                }
                m_pSecSession->RepaymentLeverageOrder(bg_date,
                                                      bg_value,
                                                      order,
                                                      pwd,
                                                      callback);
            }
            break;
        default:
            // �s���Ȗ��߂��ς܂�Ă�(error)
            return false;
        }
        //
        return true;
    }

    /*!
     *  @brief  ���߃��X�g�擪�̖��߂���������
     *  @param  investments ��������
     *  @param  aes_pwd
     *  @param  tickCount   �o�ߎ���[�~���b]
     */
    void IssueOrder(eStockInvestmentsType investments,
                    const garnet::CipherAES_string& aes_pwd,
                    int64_t tickCount)
    {
        if (m_command_list.empty()) {
            return; // ��
        }
        if (!m_wait_order.empty()) {
            return; // �҂�������
        }
        const StockTradingCommandPtr& command_ptr(m_command_list.front());
        const StockTradingCommand& command(*command_ptr);

        m_wait_order.push_back(command_ptr);
        m_command_list.pop_front();

        if (!IssueOrderCore(command, investments, aes_pwd, tickCount)) {
            // �����ł��Ȃ������猋�ʑ҂��폜(���O���ŋN���蓾��)
            m_wait_order.pop_back();
        }
    }


public:
    /*!
     *  @param  sec_session �،���ЂƂ̃Z�b�V����
     *  @param  tw_session  twitter�Ƃ̃Z�b�V����
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     */
    PIMPL(const SecuritiesSessionPtr& sec_session,
          const garnet::TwitterSessionForAuthorPtr& tw_session,
          TradeAssistantSetting& script_mng)
    : m_pSecSession(sec_session)
    , m_pTwSession(tw_session)
    , m_tactics()
    , m_tactics_link()
    , m_emergency_time_ms(
        garnet::utility_datetime::ToMiliSecondsFromSecond(script_mng.GetEmergencyCoolSecond()))
    , m_monitoring_brand()
    , m_monitoring_data()
    , m_holdings()
    , m_b_lock_odmng_and_wait_execinfo(false)
    , m_command_list()
    , m_emergency_state()
    , m_wait_order()
    , m_server_order()
    , m_exec_order()
    , m_server_order_id()
    , m_investments(INVESTMENTS_NONE)
    , m_tick_count(0)
    , m_last_tick_rcv_rep_order(0)
    {
        UpdateMessage msg;
        if (!script_mng.BuildStockTactics(msg, m_tactics, m_tactics_link)) {
            // ���s(error)
        }
    }

    /*!
     *  @brief  �،���Ђ���̕ԓ���҂��Ă邩
     *  @retval true    �������ʑ҂����Ă�
     */
    bool IsInWaitMessageFromSecurities() const
    {
        return !m_wait_order.empty();
    }
    
    /*!
     *  @brief  �Ď������R�[�h�擾
     *  @param[out] dst �i�[��
     */
    void GetMonitoringCode(StockCodeContainer& dst)
    {
        for (const auto& link: m_tactics_link) {
            dst.insert(link.first);
        }
    }

    /*!
     *  @brief  �Ď�����������
     *  @param  investments_type    ��������
     *  @param  rcv_brand_data      ��M�����Ď������Q
     *  @retval true                ����
     */
    bool InitMonitoringBrand(eStockInvestmentsType investments_type,
                             const StockBrandContainer& rcv_brand_data)
    {
        // ��M�����Ď������Q���헪�f�[�^�ƈ�v���Ă邩�`�F�b�N
        std::unordered_map<uint32_t, StockValueData> monitoring_data;
        StockCodeContainer monitoring_code;
        GetMonitoringCode(monitoring_code);
        monitoring_data.reserve(monitoring_code.size());
        for (uint32_t code: monitoring_code) {
            if (rcv_brand_data.end() != rcv_brand_data.find(code)) {
                monitoring_data.emplace(code, StockValueData(code));
            } else {
                return false;
            }
        }
        // �󂾂�����V�K�쐬
        auto itMtd = m_monitoring_data.find(investments_type);
        if (itMtd == m_monitoring_data.end()) {
            m_monitoring_data.emplace(investments_type, monitoring_data);
            m_monitoring_brand = rcv_brand_data;
        }
        return true;
    }

    /*!
     *  @brief  �Ď��������o��
     *  @param  log_dir �o�̓f�B���N�g��
     *  @param  date    �N����
     */
    void OutputMonitoringLog(const std::string& log_dir, const garnet::YYMMDD& date) const
    {
        const auto outputLog = [log_dir, date](StockValueData vdata, std::string pts_tag)
        {
            const std::string code_str(std::move(std::to_string(vdata.m_code.GetCode())));
            const std::string date_str(std::move(date.to_string()));
            vdata.OutputLog(std::move(log_dir + pts_tag + code_str + "_" + date_str + ".csv"));
        };
        const auto itPTS = m_monitoring_data.find(INVESTMENTS_PTS);
        if (itPTS != m_monitoring_data.end()) {
            for (const auto& md: itPTS->second) {
                std::thread t(outputLog, md.second, "pts_");
                t.detach();
            }
        }
        const auto itTKY = m_monitoring_data.find(INVESTMENTS_TOKYO);
        if (itTKY != m_monitoring_data.end()) {
            for (const auto& md: itTKY->second) {
                std::thread t(outputLog, md.second, std::string());
                t.detach();
            }
        }
    }
    
    /*!
     *  @brief  ���i�f�[�^�X�V
     *  @param  investments_type    ��������
     *  @param  senddate            ���i�f�[�^���M����
     *  @param  rcv_valuedata       �󂯎�������i�f�[�^
     */
    void UpdateValueData(eStockInvestmentsType investments_type,
                         const std::wstring& sendtime,
                         const std::vector<RcvStockValueData>& rcv_valuedata)
    {
        auto itMtd = m_monitoring_data.find(investments_type);
        if (itMtd != m_monitoring_data.end()) {
            garnet::sTime tm_send; // ���i�f�[�^���M����(�T�[�o�^�C��)
            auto pt(garnet::utility_datetime::ToLocalTimeFromRFC1123(sendtime));
            garnet::utility_datetime::ToTimeFromBoostPosixTime(pt, tm_send);
            auto& valuedata(itMtd->second);
            for (const auto& vunit: rcv_valuedata) {
                auto it = valuedata.find(vunit.m_code);
                if (it != valuedata.end()) {
                    it->second.UpdateValueData(vunit, tm_send);
                } else {
                    // ������Ȃ�������ǂ�����H(error)
                }
            }
        } else {
            // �Ȃ�������������ĂȂ�(error)
        }
    }

    /*!
     *  @brief  �����ςݒ�������
     *  @param  src             �����ςݒ���map(1�������)
     *  @param  user_order_id   �T�������ԍ�(�\���p)
     *  @return iterator
     */
    std::unordered_map<int32_t, StockTradingCommandPtr>::iterator
        SearchServerOrder(std::unordered_map<int32_t, StockTradingCommandPtr>& src,
                          int32_t user_order_id)
    {
        const auto itID = m_server_order_id.find(user_order_id);
        if (itID == m_server_order_id.end()) {
            return src.end(); // ID�ϊ����s(error)
        }
        int32_t order_id = itID->second;
        return src.find(order_id);
    }

    /*!
     *  @brief  ���������X�V
     *  @param  rcv_info    �󂯎���������
     */
    void UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info)
    {
        m_b_lock_odmng_and_wait_execinfo = false;

        // ��荷��(�O��X�V��ɖ�肵�����)�𓾂�
        std::vector<StockExecInfoAtOrder> diff_info;
        m_holdings.GetExecInfoDiff(rcv_info, diff_info);
        if (diff_info.empty()) {
            return ; // �ω��Ȃ�
        }
        // ��荷���ƑΉ�����u�����ςݐM�p�ԍϔ��������v�����o��
        ServerRepLevOrder rep_order;
        for (const auto& ex_info: diff_info) {
            if (ex_info.m_type != ORDER_REPBUY && ex_info.m_type != ORDER_REPSELL) {
                continue; // �Ώۂ͐M�p�ԍϔ����̂�
            }
            auto& sv_order(m_server_order[INVESTMENTS_TOKYO]);
            const int32_t user_order_id = ex_info.m_user_order_id;
            const auto itOrder = SearchServerOrder(sv_order, user_order_id);
            if (itOrder == sv_order.end()) {
                // �����ςݒ���������(error) ... (a)
                // �c�[���O�łȂ�炩�������Ă��ꍇ�͂��蓾��
                continue;
            }
            rep_order.emplace(user_order_id, itOrder->second);
        }
        // �ۗL�����Ǘ��X�V
        m_holdings.UpdateExecInfo(rcv_info, diff_info, rep_order);
        // ���ςݒ����X�V
        for (auto it = m_exec_order.begin(); it != m_exec_order.end(); it++) {
            // �R�t����"�ۗL����"���Ȃ��Ȃ��Ă���"���ςݒ���"���폜
            const StockCode s_code(it->first);
            auto itRmv = std::remove_if(it->second.begin(),
                                        it->second.end(),
                                        [this, &s_code](const StockExecOrderIdentifier& ex) {
                return !m_holdings.CheckPosition(s_code, ex.second);
            });
            if (it->second.end() != itRmv) {
                it->second.erase(itRmv, it->second.end());
            }
        }
        // �����ςݒ����X�V
        for (const auto& ex_info: diff_info) {
            auto& sv_order(m_server_order[ex_info.m_investments]);
            const int32_t user_order_id = ex_info.m_user_order_id;
            const auto itOrder = SearchServerOrder(sv_order, user_order_id);
            if (itOrder == sv_order.end()) {
                continue; // (a)�ɓ���
            }
            // ���ʒm
            OutputExecMessage(itOrder->second, ex_info);
            // �S�����
            if (ex_info.m_b_complete) {
                // ������/�V�K�M�p�����Ȃ�΁u���ςݒ����v�֓o�^
                const uint32_t code = ex_info.m_code;
                const eOrderType type = ex_info.m_type;
                if (ex_info.m_b_leverage && (type == ORDER_BUY || type == ORDER_SELL)) {
                    std::vector<int32_t> pos_id;
                    m_holdings.GetPositionID(user_order_id, pos_id);
                    m_exec_order[code].emplace_back(*itOrder->second, pos_id);
                } else if (type == ORDER_BUY) {
                    m_exec_order[code].emplace_back(*itOrder->second, std::vector<int32_t>());
                }
                // "�����ςݒ���"����폜
                sv_order.erase(itOrder);
            }
        }
    }
    /*!
     *  @brief  �ۗL�����X�V
     *  @param  spot        �����ۗL��
     *  @param  position    �M�p�ۗL��
     */
    void UpdateHoldings(const SpotTradingsStockContainer& spot,
                        const StockPositionContainer& position)
    {
        m_holdings.UpdateHoldings(spot, position);
    }

    /*!
     *  @brief  Update�֐�
     *  @param  tickCount   �o�ߎ���[�~���b]
     *  @param  now_time    ���ݎ����b
     *  @param  sec_time    ���Z�N�V�����J�n����
     *  @param  investments ��������
     *  @param  valuedata   ���i�f�[�^(1�������)
     *  @param  aes_pwd
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     */
    void Update(int64_t tickCount,
                const garnet::HHMMSS& now_time,
                const garnet::HHMMSS& sec_time,
                eStockInvestmentsType investments,
                const garnet::CipherAES_string& aes_pwd,
                TradeAssistantSetting& script_mng)
    {
        // �������ʂ��ς�����獡���閽�߃��X�getc��j��
        if (investments != m_investments) {
            m_command_list.clear();
            m_emergency_state.clear();
        }

        // �ً}���[�h��ԍX�V
        {
            const int64_t diff_time = tickCount - m_tick_count;
            auto itRmv = std::remove_if(m_emergency_state.begin(),
                                        m_emergency_state.end(),
                                        [diff_time](EmergencyModeState& emstat)
            {
                emstat.m_timer -= diff_time;
                return emstat.m_timer <= 0;
            });
            if (itRmv != m_emergency_state.end()) {
                m_emergency_state.erase(itRmv, m_emergency_state.end());
            }
        }
        // �헪����
        InterpretTactics(investments, now_time, sec_time,
                         m_monitoring_data[investments], script_mng);
        // ���ߏ���
        IssueOrder(investments, aes_pwd, tickCount);
        //
        m_tick_count = tickCount;
        m_investments = investments;
    }

};

/*!
 *  @param  sec_session �،���ЂƂ̃Z�b�V����
 *  @param  tw_session  twitter�Ƃ̃Z�b�V����
 *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
 */
StockOrderingManager::StockOrderingManager(const SecuritiesSessionPtr& sec_session,
                                           const garnet::TwitterSessionForAuthorPtr& tw_session,
                                           TradeAssistantSetting& script_mng)
: m_pImpl(new PIMPL(sec_session, tw_session, script_mng))
{
}
/*!
 */
StockOrderingManager::~StockOrderingManager()
{
}

/*!
 *  @brief  �،���Ђ���̕ԓ���҂��Ă邩
 *  @note   �������Ă�Œ��Ȃ��true
 */
bool StockOrderingManager::IsInWaitMessageFromSecurities() const
{
    return m_pImpl->IsInWaitMessageFromSecurities();
}

/*!
 *  @brief  �Ď������R�[�h�擾
 *  @param[out] dst �i�[��
 */
void StockOrderingManager::GetMonitoringCode(StockCodeContainer& dst)
{
    m_pImpl->GetMonitoringCode(dst);
}
/*!
 *  @brief  �Ď�����������
 *  @param  investments_type    ��������
 *  @param  rcv_brand_data      ��M�����Ď������Q
 */
bool StockOrderingManager::InitMonitoringBrand(eStockInvestmentsType investments_type,
                                               const StockBrandContainer& rcv_brand_data)
{
    return m_pImpl->InitMonitoringBrand(investments_type, rcv_brand_data);
}
/*!
 *  @brief  �Ď��������o��
 *  @param  log_dir �o�̓f�B���N�g��
 *  @param  date    �N����
 */
void StockOrderingManager::OutputMonitoringLog(const std::string& log_dir,
                                               const garnet::YYMMDD& date)
{
    m_pImpl->OutputMonitoringLog(log_dir, date);
}

/*!
 *  @brief  �ۗL�����X�V
 *  @param  spot        �����ۗL��
 *  @param  position    �M�p�ۗL��
 */
void StockOrderingManager::UpdateHoldings(const SpotTradingsStockContainer& spot,
                                            const StockPositionContainer& position)
{
    m_pImpl->UpdateHoldings(spot, position);
}

/*!
 *  @brief  ���i�f�[�^�X�V
 *  @param  investments_type    ��������
 *  @param  senddate            ���i�f�[�^���M����
 *  @param  rcv_valuedata       �󂯎�������i�f�[�^
 */
void StockOrderingManager::UpdateValueData(eStockInvestmentsType investments_type,
                                           const std::wstring& sendtime,
                                           const std::vector<RcvStockValueData>& rcv_valuedata)
{
    m_pImpl->UpdateValueData(investments_type, sendtime, rcv_valuedata);
}

/*!
 *  @brief  ���������X�V
 *  @param  rcv_info    �󂯎���������
 */
void StockOrderingManager::UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info)
{
    m_pImpl->UpdateExecInfo(rcv_info);
}

/*!
 *  @brief  Update�֐�
 *  @param  tickCount   �o�ߎ���[�~���b]
 *  @param  now_time    ���ݎ����b
 *  @param  sec_time    ���Z�N�V�����J�n����
 *  @param  investments ��������
 *  @param  valuedata   ���i�f�[�^(1�������)
 *  @param  aes_pwd
 *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
 */
void StockOrderingManager::Update(int64_t tickCount,
                                  const garnet::HHMMSS& now_time,
                                  const garnet::HHMMSS& sec_time,
                                  eStockInvestmentsType investments,
                                  const garnet::CipherAES_string& aes_pwd,
                                  TradeAssistantSetting& script_mng)
{
    m_pImpl->Update(tickCount, now_time, sec_time, investments, aes_pwd, script_mng);
}

} // namespace trading
