#include "DriveDownload.h"
#include "HttpModule.h"
#include "Json.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"

/*Y.Tsukioka 11.17 �O�[�O���h���C�u����̃t�@�C���_�E�����[�h�@�\����(�A�v���̃A�b�v�f�[�g�@�\�Ɏg�p)*/

ADriveDownload::ADriveDownload()
{
    // ���t���[��Tick���Ăяo���ݒ�i�p�t�H�[�}���X���K�v�ȏꍇ�͖���������ق����ǂ��j
    PrimaryActorTick.bCanEverTick = true;
}

// �O���t�@�C������N���C�A���gID�ƃV�[�N���b�g��ǂݍ���
bool ADriveDownload::LoadSecrets(FString& OutClientID, FString& OutClientSecret)
{
    FString SecretsFilePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/Secrets.txt"));
    FString FileContents;

    // �t�@�C������͂���ID�ƃV�[�N���b�g���擾
    TArray<FString> Lines;
    FileContents.ParseIntoArrayLines(Lines);

    for (const FString& Line : Lines)
    {
        if (Line.StartsWith(TEXT("ClientID=")))
        {
            OutClientID = Line.Mid(9);  // "ClientID="�̒�����9����
        }
        else if (Line.StartsWith(TEXT("ClientSecret=")))
        {
            OutClientSecret = Line.Mid(13);  // "ClientSecret="�̒�����13����
        }
    }

    if (OutClientID.IsEmpty() || OutClientSecret.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Secrets file is missing required fields."));
        return false;
    }

    return true;
}

// Google OAuth�𗘗p���ăA�N�Z�X�g�[�N�����擾
void ADriveDownload::GetAccessToken(const FString& ReceivedCode)
{
    // OAuth�N���C�A���gID�A�N���C�A���g�V�[�N���b�g�A���_�C���N�gURI�ƃg�[�N�����N�G�X�gURL�̐ݒ�
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

    // �A�N�Z�X�g�[�N���擾��HTTP���N�G�X�g���쐬
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> TokenRequest = FHttpModule::Get().CreateRequest();
    TokenRequest->SetURL(TokenRequestURL);
    TokenRequest->SetVerb("POST");

    // �g�[�N���擾���N�G�X�g�p�̃f�[�^�ݒ�
    FString PostData = FString::Printf(TEXT("code=%s&client_id=%s&client_secret=%s&redirect_uri=%s&grant_type=authorization_code"),
        *ReceivedCode, *ClientID, *ClientSecret, *RedirectURI);

    TokenRequest->SetContentAsString(PostData);
    TokenRequest->SetHeader("Content-Type", "application/x-www-form-urlencoded");

    // ���N�G�X�g�������̃R�[���o�b�N��ݒ肵�A���N�G�X�g�𑗐M
    TokenRequest->OnProcessRequestComplete().BindUObject(this, &ADriveDownload::OnTokenResponse);
    TokenRequest->ProcessRequest();
}

// �g�[�N���擾���N�G�X�g�ɑ΂��鉞���̏���
void ADriveDownload::OnTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        if (Response->GetResponseCode() == 200)  // ����ȃ��X�|���X���m�F
        {
            // JSON���X�|���X�����
            FString ResponseString = Response->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
            {
                // �A�N�Z�X�g�[�N�����擾
                FString AccessToken = JsonObject->GetStringField("access_token");
                UE_LOG(LogTemp, Log, TEXT("Access Token: %s"), *AccessToken);

                // ���t���b�V���g�[�N�����܂܂�Ă��邩�m�F���A�ۑ�
                if (JsonObject->HasField("refresh_token"))
                {
                    FString RefreshToken = JsonObject->GetStringField("refresh_token");
                    UE_LOG(LogTemp, Log, TEXT("Refresh Token: %s"), *RefreshToken);

                    // ���t���b�V���g�[�N�����t�@�C���ɕۑ����A�������������O�ɕ\��
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
                    UE_LOG(LogTemp, Warning, TEXT("No refresh token in response."));  // ���t���b�V���g�[�N�����Ȃ��ꍇ�̌x��
                }

                // �A�N�Z�X�g�[�N���𗘗p���ăt�@�C�����_�E�����[�h
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

// ���t���b�V���g�[�N���𗘗p���ăA�N�Z�X�g�[�N�����X�V
void ADriveDownload::RefreshAccessToken()
{
    // �ۑ����ꂽ���t���b�V���g�[�N�����t�@�C������ǂݍ���
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

    // ���t���b�V���g�[�N�����g�����A�N�Z�X�g�[�N���X�V���N�G�X�g
    FString PostData = FString::Printf(TEXT("client_id=%s&client_secret=%s&refresh_token=%s&grant_type=refresh_token"),
        *ClientID, *ClientSecret, *RefreshToken);
    TokenRequest->SetContentAsString(PostData);
    TokenRequest->SetHeader("Content-Type", "application/x-www-form-urlencoded");

    // �X�V���N�G�X�g�������̃R�[���o�b�N�ݒ�ƃ��N�G�X�g���M
    TokenRequest->OnProcessRequestComplete().BindUObject(this, &ADriveDownload::OnTokenResponse);
    TokenRequest->ProcessRequest();
}

// �A�N�Z�X�g�[�N�����g�p����Google�h���C�u����t�@�C�����_�E�����[�h
void ADriveDownload::StartDownload(const FString& AccessToken)
{
    // HTTP���N�G�X�g�̍쐬
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    // Google Drive API�̃t�@�C���_�E�����[�hURL�iFileID���w��j
    FString FileID = "1uUgdXBWYHAjCPT-npJf7P8HZVi1P1PMK";  // �Ώۃt�@�C��ID�������ɓ���
    FString URL = FString::Printf(TEXT("https://www.googleapis.com/drive/v3/files/%s?alt=media"), *FileID);

    // ���N�G�X�g�̏ڍאݒ�
    Request->SetURL(URL);
    Request->SetVerb("GET");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AccessToken));

    // �R�[���o�b�N�̐ݒ�ƃ��N�G�X�g�̑��M
    Request->OnProcessRequestComplete().BindUObject(this, &ADriveDownload::OnDownloadComplete);
    Request->ProcessRequest();
}

// �_�E�����[�h�������̏���
void ADriveDownload::OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        if (Response->GetResponseCode() == 200)  // 200 OK�m�F
        {
            // �_�E�����[�h�����f�[�^���o�C�g�z��Ŏ擾
            TArray<uint8> FileData = Response->GetContent();
            UE_LOG(LogTemp, Log, TEXT("File downloaded successfully. File size: %d bytes"), FileData.Num());

            // �t�@�C���ۑ�����
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

// Node.js�T�[�o�[����F�؃R�[�h���󂯎�郊�X�i�[���J�n
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

// Node.js�T�[�o�[����̔F�؃R�[�h��M���̏���
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
                // �F�؃R�[�h���擾������A���X�i�[�^�C�}�[���~
                GetWorld()->GetTimerManager().ClearTimer(AuthCodeCheckTimer);

                // �F�؃R�[�h���g�p���ăA�N�Z�X�g�[�N���擾
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

// �v���C�J�n���̏���
void ADriveDownload::BeginPlay()
{
    Super::BeginPlay();

    // 5�b���ƂɔF�؃R�[�h�̎�M���`�F�b�N
    GetWorld()->GetTimerManager().SetTimer(AuthCodeCheckTimer, this, &ADriveDownload::StartAuthCodeListener, 5.0f, true);

    FString RefreshToken;

    // ���t���b�V���g�[�N�����t�@�C���ɕۑ�����Ă��邩�m�F
    bool bIsRefreshTokenAvailable = FFileHelper::LoadFileToString(RefreshToken, TEXT("C:/Users/A23029/Desktop/SP/MyHololenz_BackUp/RefreshToken/RefreshTokenFile.txt"));

    if (bIsRefreshTokenAvailable)
    {
        UE_LOG(LogTemp, Log, TEXT("Refresh token found, refreshing access token..."));
        RefreshAccessToken();  // ���t���b�V���g�[�N���ŃA�N�Z�X�g�[�N�����X�V
    }
    else
    {
        // Node.js�T�[�o�[�ɔF�؃��N�G�X�g�𑗐M
        FString AuthRequestURL = "http://localhost:8080/auth";
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(AuthRequestURL);
        Request->SetVerb("GET");
        Request->ProcessRequest();

        StartAuthCodeListener();
    }
}

// ���t���[���̏���
void ADriveDownload::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
