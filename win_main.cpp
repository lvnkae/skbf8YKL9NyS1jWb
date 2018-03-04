// trade_assistant.cpp : �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include "Resource.h"

#include "win_main_struct.h"

#include "environment.h"
#include "trade_assistor.h"
#include "update_message.h"

#include "utility/utility_datetime.h"

#include <iostream>

class nnStreambuf : public std::wstreambuf {
public:
    virtual int_type overflow(int_type c = EOF) {
        if (c != EOF)
        {
            wchar_t buf[] = { c, '\0' };
            OutputDebugString(buf);
        }
        return c;
    }
};

class Os {
    nnStreambuf dbgstream;
    std::wstreambuf *default_stream;
public:
    Os() {
        default_stream = std::wcout.rdbuf(&dbgstream);
        // Win�̊��ˑ��̏����������
    }
    ~Os() {
        std::wcout.rdbuf(default_stream);
        // Win�̊��ˑ��̌�n���������
    }
};

// �萔
const int64_t IDC_BUTTON_READSETTING = 1000; //!< �q�E�B���h�EID�F�ݒ�ǂݍ��݃{�^��
const int64_t IDC_BUTTON_STARTTRADE = 1001;  //!< �q�E�B���h�EID�F�g���[�h�J�n
const int64_t IDC_BUTTON_PAUSETRADE = 1002;  //!< �q�E�B���h�EID�F�����ꎞ��~
const int64_t IDC_LOGDISPLAY = 1003;         //!< �q�E�B���h�EID�F���O�\��
// �萔�F�{�^���z�u�p�����[�^
const int32_t BUTTON_WIDTH = 256;
const int32_t BUTTON_HEIGHT = 30;
const int32_t BUTTON_INTERVAL = 20;
const int32_t BUTTON_POS_X_READSETTING = 10;
const int32_t BUTTON_POS_X_STARTTRADE = BUTTON_POS_X_READSETTING + BUTTON_WIDTH + BUTTON_INTERVAL;
const int32_t BUTTON_POS_X_PAUSETRADE = BUTTON_POS_X_STARTTRADE + BUTTON_WIDTH + BUTTON_INTERVAL;
const int32_t BUTTON_POS_Y = 10;
// �萔�F�ڑ�������
const int32_t TEXT_LEN_IDENTIFY = 64;
const int32_t TEXT_BUFF_LEN_IDENTIFY = TEXT_LEN_IDENTIFY+1;


// �O���[�o���ϐ�:
WinMainStruct g_WinMain;

// ���̃R�[�h ���W���[���Ɋ܂܂��֐��̐錾��]�����܂�:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK InputIdentify(HWND, UINT, WPARAM, LPARAM);

/*!
 *  @brief  WinMain
 */
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPTSTR lpCmdLine,
                       _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    Os oss;

	// �O���[�o������������������Ă��܂��B
    LoadString(hInstance, IDS_APP_TITLE, g_WinMain.m_szTitle, g_WinMain.MAX_LOADSTRING);
    LoadString(hInstance, IDC_TRADE_ASSISTANT, g_WinMain.m_szWindowClass, g_WinMain.MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// �A�v���P�[�V�����̏����������s���܂�:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

    //
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TRADE_ASSISTANT));

    // ���[�U�[����������
    //std::setlocale(LC_ALL, "ja_JP.UTF-8");
    std::shared_ptr<Environment> environment(Environment::Create());
    std::shared_ptr<trading::TradeAssistor> trade_assistant(std::make_shared<trading::TradeAssistor>());
    g_WinMain.m_TradeAssistor = trade_assistant;

	// ���C�� ���b�Z�[�W ���[�v:
    MSG msg;
    const int64_t UPDATE_INTV_MS = 32; // 1000/30
    int64_t prevTickCount = 0;
	while (true)
	{
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            // ���b�Z�[�W���擾���AWM_QUIT���ǂ���
            if (!GetMessage(&msg, nullptr, 0, 0)) {
                break;
            }
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);  //�L�[�{�[�h���p���\�ɂ���
                DispatchMessage(&msg);  //�����Windows�ɖ߂�
            }
            {
                HWND hWnd = msg.hwnd;
                HWND hwnd_btn_read(GetDlgItem(hWnd, IDC_BUTTON_READSETTING));
                HWND hwnd_btn_start(GetDlgItem(hWnd, IDC_BUTTON_STARTTRADE));
                HWND hwnd_btn_pause(GetDlgItem(hWnd, IDC_BUTTON_PAUSETRADE));
                if (hwnd_btn_read && hwnd_btn_start && hwnd_btn_pause) {
                    if (!IsWindowEnabled(hwnd_btn_read) &&
                        !IsWindowEnabled(hwnd_btn_start) &&
                        !IsWindowEnabled(hwnd_btn_pause)) {
                        if (g_WinMain.m_TradeAssistor.lock()->IsReady()) {
                            EnableWindow(hwnd_btn_start, TRUE);
                        }
                    }
                }
            }
        } else {
            // �ő�30�t���[���œ��삳���Ă݂�
            int64_t tickCount = garnet::utility_datetime::GetTickCountGeneral();
            if (tickCount - prevTickCount >= UPDATE_INTV_MS) {
                UpdateMessage trading_message;
                trade_assistant->Update(tickCount, trading_message);
                trading_message.OutputMessage();
                prevTickCount = tickCount;
            }
		}
        Sleep(1);
	}

	return static_cast<int>(msg.wParam);
}



/*!
 *  @brief  �E�B���h�E �N���X��o�^���܂��B
 */
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRADE_ASSISTANT));
	wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TRADE_ASSISTANT);
    wcex.lpszClassName = g_WinMain.m_szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

/*!
 *  @brief  �C���X�^���X �n���h����ۑ����āA���C�� �E�B���h�E���쐬���܂��B
 *  @note   ���̊֐��ŁA�O���[�o���ϐ��ŃC���X�^���X �n���h����ۑ����A
 *  @note   ���C�� �v���O���� �E�B���h�E���쐬����ѕ\�����܂��B
 */
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   g_WinMain.m_hInstance = hInstance; // �O���[�o���ϐ��ɃC���X�^���X�������i�[���܂��B

   HWND hWnd(CreateWindow(g_WinMain.m_szWindowClass,
                          g_WinMain.m_szTitle,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          0,
                          CW_USEDEFAULT,
                          0,
                          nullptr,
                          nullptr,
                          hInstance, nullptr));
   if (!hWnd) {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  �֐�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  �ړI:    ���C�� �E�B���h�E�̃��b�Z�[�W���������܂��B
//
//  WM_COMMAND	- �A�v���P�[�V���� ���j���[�̏���
//  WM_PAINT	- ���C�� �E�B���h�E�̕`��
//  WM_DESTROY	- ���~���b�Z�[�W��\�����Ė߂�
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_CREATE:
        {
            CreateWindow(TEXT("BUTTON"),
                         L"�ݒ�t�@�C���ǂݍ���",
                         WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                         BUTTON_POS_X_READSETTING, BUTTON_POS_Y,
                         BUTTON_WIDTH, BUTTON_HEIGHT,
                         hWnd, reinterpret_cast<HMENU>(IDC_BUTTON_READSETTING), g_WinMain.m_hInstance, nullptr);
            CreateWindow(TEXT("BUTTON"),
                         L"�g���[�h�J�n",
                         WS_CHILD|WS_VISIBLE|WS_DISABLED|BS_DEFPUSHBUTTON,
                         BUTTON_POS_X_STARTTRADE, BUTTON_POS_Y,
                         BUTTON_WIDTH, BUTTON_HEIGHT,
                         hWnd, reinterpret_cast<HMENU>(IDC_BUTTON_STARTTRADE), g_WinMain.m_hInstance, nullptr);
            CreateWindow(TEXT("BUTTON"),
                         L"�����ꎞ��~",
                         WS_CHILD|WS_VISIBLE|WS_DISABLED|BS_DEFPUSHBUTTON,
                         BUTTON_POS_X_PAUSETRADE, BUTTON_POS_Y,
                         BUTTON_WIDTH, BUTTON_HEIGHT,
                         hWnd, reinterpret_cast<HMENU>(IDC_BUTTON_PAUSETRADE), g_WinMain.m_hInstance, nullptr);
        }
	case WM_COMMAND:
        {
            int32_t wmId = LOWORD(wParam);
		    int32_t wmEvent = HIWORD(wParam);
		    // �I�����ꂽ���j���[�̉��:
		    switch (wmId)
		    {
		    case IDM_ABOUT:
                DialogBox(g_WinMain.m_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			    break;
		    case IDM_EXIT:
			    DestroyWindow(hWnd);
			    break;
            case IDC_BUTTON_READSETTING:
                {
                    HWND hwnd_btn_read(GetDlgItem(hWnd, IDC_BUTTON_READSETTING));
                    if (hwnd_btn_read) {
                        EnableWindow(hwnd_btn_read, FALSE);
                        g_WinMain.m_TradeAssistor.lock()->ReadSetting();
                    }
                }
                break;
            case IDC_BUTTON_STARTTRADE:
                {
                    DialogBox(g_WinMain.m_hInstance, MAKEINTRESOURCE(IDD_STARTTRADE), hWnd, InputIdentify);
                }
                break;
            case IDC_BUTTON_PAUSETRADE:
                {
                    HWND hwnd_btn_pause(GetDlgItem(hWnd, IDC_BUTTON_PAUSETRADE));
                    if (hwnd_btn_pause) {
                        EnableWindow(hwnd_btn_pause, FALSE);
                        g_WinMain.m_TradeAssistor.lock()->Pause();
                    }
                }
                break;
		    default:
			    return DefWindowProc(hWnd, message, wParam, lParam);
		    }
        }
		break;
	case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
		    // TODO: �`��R�[�h�������ɒǉ����Ă�������...
		    EndPaint(hWnd, &ps);
        }
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*!
 *  @brief   �_�C�A���O�R�[���o�b�N
 *  @note   �u�o�[�W�������v�_�C�A���O�ɑΉ�
 */
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return static_cast<INT_PTR>(TRUE);

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return static_cast<INT_PTR>(TRUE);
		}
		break;
	}
	return static_cast<INT_PTR>(FALSE);
}

/*!
 *  @brief  �_�C�A���O�R�[���o�b�N
 *  @note   �g���[�h�J�n�{�^�������������̃��O�C�������̓_�C�A���O�ɑΉ�
 */
INT_PTR CALLBACK InputIdentify(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return static_cast<INT_PTR>(TRUE);

    case WM_COMMAND:
        {
            int32_t idc(LOWORD(wParam));
            switch (idc)
            {
            case IDOK:
                {
                    HWND hWnd(GetParent(hDlg));
                    HWND hwnd_btn_start(GetDlgItem(hWnd, IDC_BUTTON_STARTTRADE));
                    HWND hwnd_btn_pause(GetDlgItem(hWnd, IDC_BUTTON_PAUSETRADE));
                    if (hWnd && hwnd_btn_start) {
                        EnableWindow(hwnd_btn_start, FALSE);
                        EnableWindow(hwnd_btn_pause, TRUE);
                        WCHAR text_buf[TEXT_BUFF_LEN_IDENTIFY];
                        GetDlgItemText(hDlg, IDC_EDIT_USERID, text_buf, static_cast<int>(sizeof(text_buf)));
                        std::wstring uid(text_buf);
                        GetDlgItemText(hDlg, IDC_EDIT_PASSWORD, text_buf, static_cast<int>(sizeof(text_buf)));
                        std::wstring pwd(text_buf);
                        GetDlgItemText(hDlg, IDC_EDIT_PASSWORD_SUB, text_buf, static_cast<int>(sizeof(text_buf)));
                        std::wstring pwd_sub(text_buf);
                        g_WinMain.m_TradeAssistor.lock()->Start(GetTickCount(), uid, pwd, pwd_sub);
                    }
                }
                EndDialog(hDlg, LOWORD(wParam));
                return static_cast<INT_PTR>(TRUE);
            case IDCANCEL:
                EndDialog(hDlg, LOWORD(wParam));
                return static_cast<INT_PTR>(TRUE);
            default:
                break;
            }
        }
        break;
    }
    return static_cast<INT_PTR>(FALSE);
}
