#include "DriveDownload.h"
#include "HttpModule.h"
#include "Json.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"

/*Y.Tsukioka 11.17 グーグルドライブからのファイルダウンロード機能実装(アプリのアップデート機能に使用)*/

ADriveDownload::ADriveDownload()
{
    // 毎フレームTickを呼び出す設定（パフォーマンスが必要な場合は無効化するほうが良い）
    PrimaryActorTick.bCanEverTick = true;
}

// 外部ファイルからクライアントIDとシークレットを読み込む
bool ADriveDownload::LoadSecrets(FString& OutClientID, FString& OutClientSecret)
{
    FString SecretsFilePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/Secrets.txt"));
    FString FileContents;

    // ファイルを解析してIDとシークレットを取得
    TArray<FString> Lines;
    FileContents.ParseIntoArrayLines(Lines);

    for (const FString& Line : Lines)
    {
        if (Line.StartsWith(TEXT("ClientID=")))
        {
            OutClientID = Line.Mid(9);  // "ClientID="の長さは9文字
        }
        else if (Line.StartsWith(TEXT("ClientSecret=")))
        {
            OutClientSecret = Line.Mid(13);  // "ClientSecret="の長さは13文字
        }
    }

    if (OutClientID.IsEmpty() || OutClientSecret.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Secrets file is missing required fields."));
        return false;
    }

    return true;
}

// Google OAuthを利用してアクセストークンを取得
void ADriveDownload::GetAccessToken(const FString& ReceivedCode)
{
    // OAuthクライアントID、クライアントシークレット、リダイレクトURIとトークンリクエストURLの設定
    FString ClientID, ClientSecret;

    if (!LoadSecrets(ClientID, ClientSecret))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load client secrets. Cannot proceed with token request."));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Loaded ClientID: %s"), *ClientID);
    UE_LOG(LogTemp, Log, TEXT("Loaded ClientSecret: %s"), *ClientSecret);

    FString RedirectURI = "http://localhost:8080/callback";
    FString TokenRequestURL = "https://oauth2.googleapis.com/token";

    // アクセストークン取得のHTTPリクエストを作成
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> TokenRequest = FHttpModule::Get().CreateRequest();
    TokenRequest->SetURL(TokenRequestURL);
    TokenRequest->SetVerb("POST");

    // トークン取得リクエスト用のデータ設定
    FString PostData = FString::Printf(TEXT("code=%s&client_id=%s&client_secret=%s&redirect_uri=%s&grant_type=authorization_code"),
        *ReceivedCode, *ClientID, *ClientSecret, *RedirectURI);

    TokenRequest->SetContentAsString(PostData);
    TokenRequest->SetHeader("Content-Type", "application/x-www-form-urlencoded");

    // リクエスト完了時のコールバックを設定し、リクエストを送信
    TokenRequest->OnProcessRequestComplete().BindUObject(this, &ADriveDownload::OnTokenResponse);
    TokenRequest->ProcessRequest();
}

// トークン取得リクエストに対する応答の処理
void ADriveDownload::OnTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        if (Response->GetResponseCode() == 200)  // 正常なレスポンスか確認
        {
            // JSONレスポンスを解析
            FString ResponseString = Response->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
            {
                // アクセストークンを取得
                FString AccessToken = JsonObject->GetStringField("access_token");
                UE_LOG(LogTemp, Log, TEXT("Access Token: %s"), *AccessToken);

                // リフレッシュトークンが含まれているか確認し、保存
                if (JsonObject->HasField("refresh_token"))
                {
                    FString RefreshToken = JsonObject->GetStringField("refresh_token");
                    UE_LOG(LogTemp, Log, TEXT("Refresh Token: %s"), *RefreshToken);

                    // リフレッシュトークンをファイルに保存し、成功したかログに表示
                    bool bSaveSuccess = FFileHelper::SaveStringToFile(RefreshToken, TEXT("C:/Users/A23029/Desktop/SP/MyHololenz_BackUp/RefreshToken/RefreshTokenFile.txt"));
                    if (bSaveSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Refresh token saved successfully."));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to save refresh token."));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("No refresh token in response."));  // リフレッシュトークンがない場合の警告
                }

                // アクセストークンを利用してファイルをダウンロード
                StartDownload(AccessToken);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response: %s"), *ResponseString);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to obtain access token. HTTP Response Code: %d"), Response->GetResponseCode());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to obtain access token. Request was not successful."));
    }
}

// リフレッシュトークンを利用してアクセストークンを更新
void ADriveDownload::RefreshAccessToken()
{
    // 保存されたリフレッシュトークンをファイルから読み込む
    FString RefreshToken, ClientID, ClientSecret;;
    FFileHelper::LoadFileToString(RefreshToken, TEXT("C:/Users/A23029/Desktop/SP/MyHololenz_BackUp/RefreshToken/RefreshTokenFile.txt"));

    if (!LoadSecrets(ClientID, ClientSecret))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load client secrets. Cannot proceed with token request."));
        return;
    }
    FString TokenRequestURL = "https://oauth2.googleapis.com/token";

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> TokenRequest = FHttpModule::Get().CreateRequest();
    TokenRequest->SetURL(TokenRequestURL);
    TokenRequest->SetVerb("POST");

    // リフレッシュトークンを使ったアクセストークン更新リクエスト
    FString PostData = FString::Printf(TEXT("client_id=%s&client_secret=%s&refresh_token=%s&grant_type=refresh_token"),
        *ClientID, *ClientSecret, *RefreshToken);
    TokenRequest->SetContentAsString(PostData);
    TokenRequest->SetHeader("Content-Type", "application/x-www-form-urlencoded");

    // 更新リクエスト完了時のコールバック設定とリクエスト送信
    TokenRequest->OnProcessRequestComplete().BindUObject(this, &ADriveDownload::OnTokenResponse);
    TokenRequest->ProcessRequest();
}

// アクセストークンを使用してGoogleドライブからファイルをダウンロード
void ADriveDownload::StartDownload(const FString& AccessToken)
{
    // HTTPリクエストの作成
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    // Google Drive APIのファイルダウンロードURL（FileIDを指定）
    FString FileID = "1uUgdXBWYHAjCPT-npJf7P8HZVi1P1PMK";  // 対象ファイルIDをここに入力
    FString URL = FString::Printf(TEXT("https://www.googleapis.com/drive/v3/files/%s?alt=media"), *FileID);

    // リクエストの詳細設定
    Request->SetURL(URL);
    Request->SetVerb("GET");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AccessToken));

    // コールバックの設定とリクエストの送信
    Request->OnProcessRequestComplete().BindUObject(this, &ADriveDownload::OnDownloadComplete);
    Request->ProcessRequest();
}

// ダウンロード完了時の処理
void ADriveDownload::OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        if (Response->GetResponseCode() == 200)  // 200 OK確認
        {
            // ダウンロードしたデータをバイト配列で取得
            TArray<uint8> FileData = Response->GetContent();
            UE_LOG(LogTemp, Log, TEXT("File downloaded successfully. File size: %d bytes"), FileData.Num());

            // ファイル保存処理
            FString FilePath = TEXT("C:/Users/A23029/Desktop/SP/MyHololenz_BackUp/DriveDownload/DriveofTest.txt");
            if (FFileHelper::SaveArrayToFile(FileData, *FilePath))
            {
                UE_LOG(LogTemp, Log, TEXT("File saved successfully at %s"), *FilePath);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to save file at %s"), *FilePath);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to download file. HTTP Response Code: %d"), Response->GetResponseCode());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to download file. Request was not successful."));
    }
}

// Node.jsサーバーから認証コードを受け取るリスナーを開始
void ADriveDownload::StartAuthCodeListener()
{
    FString AuthCodeReceiveURL = "http://localhost:3000/get-auth-code";
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(AuthCodeReceiveURL);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindUObject(this, &ADriveDownload::OnAuthCodeReceived);
    Request->ProcessRequest();
}

// Node.jsサーバーからの認証コード受信時の処理
void ADriveDownload::OnAuthCodeReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        FString ResponseString = Response->GetContentAsString();
        UE_LOG(LogTemp, Log, TEXT("Response from server: %s"), *ResponseString);

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseString);

        if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject->HasField("code"))
        {
            FString ReceivedCode = JsonObject->GetStringField("code");
            UE_LOG(LogTemp, Log, TEXT("Received Auth Code: %s"), *ReceivedCode);

            if (!ReceivedCode.IsEmpty())
            {
                // 認証コードを取得したら、リスナータイマーを停止
                GetWorld()->GetTimerManager().ClearTimer(AuthCodeCheckTimer);

                // 認証コードを使用してアクセストークン取得
                GetAccessToken(ReceivedCode);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Received an empty auth code."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Field 'code' not found or JSON parse failed. Response: %s"), *ResponseString);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to request auth code from server."));
    }
}

// プレイ開始時の処理
void ADriveDownload::BeginPlay()
{
    Super::BeginPlay();

    // 5秒ごとに認証コードの受信をチェック
    GetWorld()->GetTimerManager().SetTimer(AuthCodeCheckTimer, this, &ADriveDownload::StartAuthCodeListener, 5.0f, true);

    FString RefreshToken;

    // リフレッシュトークンがファイルに保存されているか確認
    bool bIsRefreshTokenAvailable = FFileHelper::LoadFileToString(RefreshToken, TEXT("C:/Users/A23029/Desktop/SP/MyHololenz_BackUp/RefreshToken/RefreshTokenFile.txt"));

    if (bIsRefreshTokenAvailable)
    {
        UE_LOG(LogTemp, Log, TEXT("Refresh token found, refreshing access token..."));
        RefreshAccessToken();  // リフレッシュトークンでアクセストークンを更新
    }
    else
    {
        // Node.jsサーバーに認証リクエストを送信
        FString AuthRequestURL = "http://localhost:8080/auth";
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(AuthRequestURL);
        Request->SetVerb("GET");
        Request->ProcessRequest();

        StartAuthCodeListener();
    }
}

// 毎フレームの処理
void ADriveDownload::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
