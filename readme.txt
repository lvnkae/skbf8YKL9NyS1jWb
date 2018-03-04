★ビルドを通すために必要なこと
    1. 依存するライブラリのインストール
       ()内は動作確認したversion
        boost (vc120.1.64.0/1.66.0, vc140.1.66.0) NuGet
        Crypto++ (5.6.5)
        C++ REST SDK (vc120.2.9.1, vc140.1.66.0) NuGet
        python (2.7.13)
            ※3.x系でもstdfx.hのバージョン設定を変えればビルドは通る
            ※スクリプト(.py)が対応していないので動作はしない
        lua (5.3)
        garnet-lib
            https://github.com/lvnkae/garnet-lib
        
    2. 依存するライブラリのビルド
        Crypto++
        garnet-lib

    3. パス設定修正
        プロパティ>C/C++>追加のインストールディレクトリ
                  >リンカー>追加のライブラリディレクトリ
                  
★要求ビルド環境について
    C++11対応されたVisualStudio
        .slnは2013用だが2015に移行済み
        win_main以外はWindows依存してないつもりなのでlinux環境でも通るはず(呼び出し部分とmakefileを作れば)
