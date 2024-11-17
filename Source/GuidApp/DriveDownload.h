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
    // �R���X�g���N�^
    ADriveDownload();

protected:
    // �Q�[���J�n���܂��̓X�|�[�����ɌĂяo�����
    virtual void BeginPlay() override;

    // ���t���[���Ăяo�����
    virtual void Tick(float DeltaTime) override;

private:
   
    // �A�N�Z�X�g�[�N�����擾����֐�
    // �w�肳�ꂽ�F�؃R�[�h���g�p���ăA�N�Z�X�g�[�N�����擾
    void GetAccessToken(const FString& ReceivedCode);

    /// <summary>
    /// �g�[�N���̎擾�ɑ΂���HTTP���X�|���X�̃R�[���o�b�N�֐�
    /// �����܂��͎��s�ɉ����ď���������
    /// </summary>
    /// <param name="Request">HTTP���N�G�X�g�I�u�W�F�N�g</param>
    /// <param name="Response">HTTP���X�|���X�I�u�W�F�N�g</param>
    /// <param name="bWasSuccessful">���N�G�X�g�������������ǂ���</param>
    void OnTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    /// <summary>
    /// �t�@�C���_�E�����[�h�̊������ɌĂяo�����R�[���o�b�N�֐�
    /// ���X�|���X�f�[�^���������A�K�v�ł���΃G���[�n���h�����O���s��
    /// </summary>
    /// <param name="Request">HTTP���N�G�X�g�I�u�W�F�N�g</param>
    /// <param name="Response">HTTP���X�|���X�I�u�W�F�N�g</param>
    /// <param name="bWasSuccessful">���N�G�X�g�������������ǂ���</param>
    void OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // �w�肳�ꂽFileID���g�p���ăt�@�C�����_�E�����[�h����֐�
    // FileID��p����HTTP���N�G�X�g���쐬���A�_�E�����[�h���J�n
    void StartDownload(const FString& FileID);

    // ���t���b�V���g�[�N�����g�p���ăA�N�Z�X�g�[�N�����X�V����֐�
    // �����̃��t���b�V���g�[�N�����g�p���āA�V�����A�N�Z�X�g�[�N�������N�G�X�g
    void RefreshAccessToken();

    // �F�؃R�[�h�̎擾���N�G�X�g���J�n����֐�
    // �F�؃R�[�h�����b�X�����A�w��̃G���h�|�C���g����R�[�h���擾
    void StartAuthCodeListener();

    /// <summary>
    /// �F�؃R�[�h�̎�M�ɑ΂���HTTP���X�|���X�̃R�[���o�b�N�֐�
    /// ���������ꍇ�A�A�N�Z�X�g�[�N���̎擾�v���Z�X���J�n
    /// </summary>
    /// <param name="Request">HTTP���N�G�X�g�I�u�W�F�N�g/param>
    /// <param name="Response">HTTP���X�|���X�I�u�W�F�N�g</param>
    /// <param name="bWasSuccessful">���N�G�X�g�������������ǂ���</param>
    void OnAuthCodeReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // �O���t�@�C������N���C�A���gID�ƃV�[�N���b�g��ǂݍ���
    bool LoadSecrets(FString& OutClientID, FString& OutClientSecret);

    // �F�؃R�[�h�̊m�F�v���Z�X�Ɏg�p����^�C�}�[�̃n���h��
    FTimerHandle AuthCodeCheckTimer;

};

