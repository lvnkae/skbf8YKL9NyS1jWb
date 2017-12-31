TradeType = "STOCK" -- 取引種別(STOCK/FX/BITCOINなど)
TradeSec = "SBI" -- 証券会社種別(SBI/KABUDOT/MONEX/NOMURAなど)

SessionKeepMinute = 20 -- 無アクセスで取引サイトとのセッションを維持できる時間[分]

StockValueIntervalSecond = 10 -- 株価更新(取得)間隔[秒]
EmergencyCoolSecond = 300 -- 緊急モード維持秒数(=冷却期間)[秒]

MaxPortfolioEntry = 50  -- 監視銘柄最大登録数
UsePortfolioNumber = 0  -- 監視銘柄を登録するポートフォリオ番号


-- 日本市場固有休業日((土日祝でなくとも休みになる月日)
JPXHoliday = {
    "12/31", "01/02", "01/03"
}

-- 株タイムテーブル
StockTimeTable = {
    --  開始時刻    モード(IDLE:待機/PTS:PTS売買/TOKYO:東証売買/CLOSED:閉場)
    {   "08:15",    "IDLE"  },  -- 待機時間[LOGIN/SET_TICKER_LIST<PTS>]
    {   "08:20",    "PTS"   },  -- PTS(朝)08:20～08:57
                                --  PTS(デイタイム)は08:20～16:00で休みなく稼働しているが東証メインにしたいのでPTS売買TTは小刻み。
    {   "08:57",    "IDLE"  },  -- 待機時間[SET_TICKER_LIST<TOKYO>]
    {   "09:00",    "TOKYO" },  -- 東証前場
    {   "11:30",    "CLOSED"},  -- 東証昼休み
    {   "12:27",    "IDLE"  },  -- 待機時間[LOGIN]
    {   "12:30",    "TOKYO" },  -- 東証後場
    {   "15:00",    "PTS"   },  -- PTS(夕)15:00～16:00 [SET_TICKER_LIST<PTS>]
                                --  東証閉場後はPTSに切り替える
    {   "16:00",    "CLOSED"},  -- 閉場(東証もPTSも閉まってる時間帯)
    {   "16:57",    "IDLE"  },  -- 待機時間[LOGIN/SET_TICKER_LIST<PTS>]
    {   "17:00",    "PTS"   },  -- PTS(夜)17:00～23:59
    {   "23:58",    "CLOSED"},  --  ギリギリまで売買する意義はないので1分前で終わる
}

-- 株売買戦略(サンプル)
StockTactics = {
    {
        Code = { 9408 },
        -- 緊急モード条件(=発注リセット&注文一時停止)
        Emergency = {
            -- 急変動対策
            {
                Type = "ValueGap", -- 最新N秒で指定割合価格変化(高値と安値の差で判定)があったか？
                Percent = 3.00,
                Second = 60,
            },
            {
                Type = "ValueGap",
                Percent = 4.00,
                Second = 180,
            },
            {
                Type = "ValueGap",
                Percent = 5.00,
                Second = 300,
            },
            -- 場中特買い/特売り対策
            {
                Type = "NoContract", -- 指定秒数無約定が続いたか？(特売/買とみなす)
                Second = 1800,
            },
        },
        -- 新規注文
        Fresh = {
            -- 注文は複数記述できる
            {
                -- GroupID
                --- 未指定ならば0
                ---- 同Tactics別GroupID → パラレルに発注
                ---- 同Tactics同GroupID → 後(下)勝ち(発注済みなら注文訂正)される[予定]
                GroupID = 9408+0,
                -- 現物買Buy/信用買BuyLev/信用売SellLev
                Type = "BuyLev",
                -- 発注価格(決定関数)
                Value = (function(v, high, low, yesterday)
                            -- 安値とlimitの低い方を採用
                            limit = 777
                            if (low < limit) then
                                return low
                            else
                                return limit
                            end
                        end),
                -- 発注株数
                Volume = 100,
                -- 発注条件
                Condition = {
                    Type = "Formula", -- lua関数で判定
                    Formula = (function(v, high, low, yesterday)
                                    -- 現値がlimitを下回ったら発注
                                    limit = 888
                                    if (v < limit) then
                                        return true
                                    else
                                        return false
                                    end
                                end),
                },
            },
            {
                GroupID = 9408+1,
                -- 現物買Buy/信用買BuyLev/信用売SellLev
                Type = "BuyLev",
                -- 発注価格(決定関数)
                Value = (function(v, high, low, yesterday)
                            -- 安値とlimitの低い方を採用
                            limit = 666
                            if (low < limit) then
                                return low
                            else
                                return limit
                            end
                        end),
                -- 発注株数
                Volume = 100,
                -- 発注条件
                Condition = {
                    Type = "Formula", -- lua関数で判定
                    Formula = (function(v, high, low, yesterday)
                                -- 現値がlimitを下回ったら発注
                                limit = 777
                                if (v < limit) then
                                    return true
                                else
                                    return false
                                end
                            end),
                },
            },
        },
        -- 返済注文
        Repayment = {
            {
                -- 現物売Sell/信返売RepSell/信返買RepBuy
                Type = "RepSell",
                -- 返済価格(決定関数)
                Value = (function(v, high, low, yesterday)
                            -- 高値+diff
                            diff = 50
                            return high + diff
                        end),
                -- 返済株数(負数なら全部)
                Volume = -1,
                -- 返済条件
                Condition = {
                    Type = "Formula", -- lua関数で判定
                    Formula = (function(v, high, low, yesterday)
                                -- 現値がlimitを上回ったら発注
                                limit = 666
                                if (v >= limit) then
                                    return true
                                else
                                    return false
                                end
                            end),
                }
            }
        }
    },
    --
    {
        -- Codeは複数記述できる
        -- (同じ戦略を複数の銘柄に適用)
        Code = { 2208, 2220, 2221 },
        -- 
        Fresh = {
            {
                -- 現物買Buy/信用買BuyLev/信用売SellLev
                Type = "BuyLev",
                -- 発注価格(決定関数)
                Value = (function(v, high, low, yesterday)
                            -- 価格が負なら成行注文
                            return -1
                        end),
                -- 発注株数
                Volume = 100,
                -- 発注条件
                Condition = {
                    Type = "ValueGap", -- 指定秒数以下で指定割合価格変化
                    Percent = -5.00,
                    Second = 60*5,
                },
            },
            {
                -- 現物買Buy/信用買BuyLev/信用売SellLev
                Type = "BuyLev",
                -- 発注価格(決定関数)
                Value = (function(v, high, low, yesterday)
                            -- 価格が負なら成行注文
                            return -1
                        end),
                -- 発注株数
                Volume = 100,
                -- 発注条件
                Condition = {
                    Type = "ValueGap", -- 指定秒数以下で指定割合価格変化
                    Percent = -6.00,
                    Second = 60*6,
                }
            },
        }
    },
}