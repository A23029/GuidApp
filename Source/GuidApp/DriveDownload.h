// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HttpModule.h"
#include "DriveDownload.generated.h"


UCLASS()
class GUIDAPP_API ADriveDownload : public AActor
{
	GENERATED_BODY()

public:
    // コンストラクタ
    ADriveDownload();

protected:
    // ゲーム開始時またはスポーン時に呼び出される
    virtual void BeginPlay() override;

    // 毎フレーム呼び出される
    virtual void Tick(float DeltaTime) override;

private:
   
    // アクセストークンを取得する関数
    // 指定された認証コードを使用してアクセストークンを取得
    void GetAccessToken(const FString& ReceivedCode);

    /// <summary>
    /// トークンの取得に対するHTTPレスポンスのコールバック関数
    /// 成功または失敗に応じて処理をする
    /// </summary>
    /// <param name="Request">HTTPリクエストオブジェクト</param>
    /// <param name="Response">HTTPレスポンスオブジェクト</param>
    /// <param name="bWasSuccessful">リクエストが成功したかどうか</param>
    void OnTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    /// <summary>
    /// ファイルダウンロードの完了時に呼び出されるコールバック関数
    /// レスポンスデータを処理し、必要であればエラーハンドリングを行う
    /// </summary>
    /// <param name="Request">HTTPリクエストオブジェクト</param>
    /// <param name="Response">HTTPレスポンスオブジェクト</param>
    /// <param name="bWasSuccessful">リクエストが成功したかどうか</param>
    void OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // 指定されたFileIDを使用してファイルをダウンロードする関数
    // FileIDを用いてHTTPリクエストを作成し、ダウンロードを開始
    void StartDownload(const FString& FileID);

    // リフレッシュトークンを使用してアクセストークンを更新する関数
    // 既存のリフレッシュトークンを使用して、新しいアクセストークンをリクエスト
    void RefreshAccessToken();

    // 認証コードの取得リクエストを開始する関数
    // 認証コードをリッスンし、指定のエンドポイントからコードを取得
    void StartAuthCodeListener();

    /// <summary>
    /// 認証コードの受信に対するHTTPレスポンスのコールバック関数
    /// 成功した場合、アクセストークンの取得プロセスを開始
    /// </summary>
    /// <param name="Request">HTTPリクエストオブジェクト/param>
    /// <param name="Response">HTTPレスポンスオブジェクト</param>
    /// <param name="bWasSuccessful">リクエストが成功したかどうか</param>
    void OnAuthCodeReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // 外部ファイルからクライアントIDとシークレットを読み込む
    bool LoadSecrets(FString& OutClientID, FString& OutClientSecret);

    // 認証コードの確認プロセスに使用するタイマーのハンドル
    FTimerHandle AuthCodeCheckTimer;

};

