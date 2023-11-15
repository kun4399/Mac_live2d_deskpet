﻿/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppModel.hpp"
#include <vector>
#include <CubismModelSettingJson.hpp>
#include <Motion/CubismMotion.hpp>
#include <Physics/CubismPhysics.hpp>
#include <CubismDefaultParameterId.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>
#include <Utils/CubismString.hpp>
#include <Id/CubismIdManager.hpp>
#include <Motion/CubismMotionQueueEntry.hpp>
#include "LAppDefine.hpp"
#include "LAppPal.hpp"
#include "LAppTextureManager.hpp"
#include "Log_util.h"
#include "LAppDelegate.hpp"
#define DRAG_SCALE 0.3f
using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::DefaultParameterId;
using namespace LAppDefine;

namespace {
    csmByte *CreateBuffer(const csmChar *path, csmSizeInt *size) {
        CF_LOG_DEBUG("create buffer: %s", path);
        return LAppPal::LoadFileAsBytes(path, size);
    }

    void DeleteBuffer(csmByte *buffer, const csmChar *path = "") {
        CF_LOG_DEBUG("delete buffer: %s", path);
        LAppPal::ReleaseBytes(buffer);
    }
}

LAppModel::LAppModel()
        : CubismUserModel(), _modelSetting(nullptr), _userTimeSeconds(0.0f) {
    if (DebugLogEnable) {
        _debugMode = true;
    }

    _idParamAngleX = CubismFramework::GetIdManager()->GetId(ParamAngleX);
    _idParamAngleY = CubismFramework::GetIdManager()->GetId(ParamAngleY);
    _idParamAngleZ = CubismFramework::GetIdManager()->GetId(ParamAngleZ);
    _idParamBodyAngleX = CubismFramework::GetIdManager()->GetId(ParamBodyAngleX);
    _idParamEyeBallX = CubismFramework::GetIdManager()->GetId(ParamEyeBallX);
    _idParamEyeBallY = CubismFramework::GetIdManager()->GetId(ParamEyeBallY);
}

LAppModel::~LAppModel() {
    _renderBuffer.DestroyOffscreenFrame();

    ReleaseMotions();
    ReleaseExpressions();
    if (_modelSetting) {
        for (csmInt32 i = 0; i < _modelSetting->GetMotionGroupCount(); i++) {
            const csmChar *group = _modelSetting->GetMotionGroupName(i);
            ReleaseMotionGroup(group);
        }
        delete (_modelSetting);
    }
}

bool LAppModel::LoadAssets(const csmChar *dir, const csmChar *fileName) {
    _modelHomeDir = dir;

    CF_LOG_DEBUG("load model setting: %s", fileName);

    csmSizeInt size;
    const csmString path = csmString(dir) + fileName;

    csmByte *buffer = CreateBuffer(path.GetRawString(), &size);
    if (buffer == nullptr) {
        return false;
    }
    ICubismModelSetting *setting = new CubismModelSettingJson(buffer, size);
    DeleteBuffer(buffer, path.GetRawString());

    SetupModel(setting);

    CreateRenderer();

    SetupTextures();
    return true;
}


bool LAppModel::SetupModel(ICubismModelSetting *setting) {
    _updating = true;
    _initialized = false;

    _modelSetting = setting;

    csmByte *buffer;
    csmSizeInt size;

    //Cubism Model
    if (strcmp(_modelSetting->GetModelFileName(), "") != 0) {
        csmString path = _modelSetting->GetModelFileName();
        path = _modelHomeDir + path;

        CF_LOG_DEBUG("create model: %s", path.GetRawString());

        buffer = CreateBuffer(path.GetRawString(), &size);
        if (buffer == nullptr) {
            return false;
        }
        LoadModel(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    //Expression
    if (_modelSetting->GetExpressionCount() > 0) {
        const csmInt32 count = _modelSetting->GetExpressionCount();
        for (csmInt32 i = 0; i < count; i++) {
            csmString name = _modelSetting->GetExpressionName(i);
            csmString path = _modelSetting->GetExpressionFileName(i);
            path = _modelHomeDir + path;

            buffer = CreateBuffer(path.GetRawString(), &size);
            if (buffer == nullptr) {
                return false;
            }

            ACubismMotion *motion = LoadExpression(buffer, size, name.GetRawString());

            if (_expressions[name] != nullptr) {
                ACubismMotion::Delete(_expressions[name]);
                _expressions[name] = nullptr;
            }
            _expressions[name] = motion;

            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    //Physics
    if (strcmp(_modelSetting->GetPhysicsFileName(), "") != 0) {
        csmString path = _modelSetting->GetPhysicsFileName();
        path = _modelHomeDir + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        if (buffer) {
            LoadPhysics(buffer, size);
            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    //Pose
    if (strcmp(_modelSetting->GetPoseFileName(), "") != 0) {
        csmString path = _modelSetting->GetPoseFileName();
        path = _modelHomeDir + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        if (buffer) {
            LoadPose(buffer, size);
            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    //EyeBlink
    if (_modelSetting->GetEyeBlinkParameterCount() > 0) {
        _eyeBlink = CubismEyeBlink::Create(_modelSetting);
    }

    //Breath
    {
        _breath = CubismBreath::Create();

        csmVector<CubismBreath::BreathParameterData> breathParameters;

        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleX, 0.0f, 15.0f, 6.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleY, 0.0f, 8.0f, 3.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleZ, 0.0f, 10.0f, 5.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamBodyAngleX, 0.0f, 4.0f, 15.5345f, 0.5f));
        breathParameters.PushBack(
                CubismBreath::BreathParameterData(CubismFramework::GetIdManager()->GetId(ParamBreath), 0.5f, 0.5f,
                                                  3.2345f, 0.5f));

        _breath->SetParameters(breathParameters);
    }

    //UserData
    if (strcmp(_modelSetting->GetUserDataFile(), "") != 0) {
        csmString path = _modelSetting->GetUserDataFile();
        path = _modelHomeDir + path;
        buffer = CreateBuffer(path.GetRawString(), &size);
        if (buffer) {
            LoadUserData(buffer, size);
            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    // EyeBlinkIds
    {
        csmInt32 eyeBlinkIdCount = _modelSetting->GetEyeBlinkParameterCount();
        for (csmInt32 i = 0; i < eyeBlinkIdCount; ++i) {
            _eyeBlinkIds.PushBack(_modelSetting->GetEyeBlinkParameterId(i));
        }
    }

    // LipSyncIds
    {
        csmInt32 lipSyncIdCount = _modelSetting->GetLipSyncParameterCount();
        for (csmInt32 i = 0; i < lipSyncIdCount; ++i) {
            _lipSyncIds.PushBack(_modelSetting->GetLipSyncParameterId(i));
        }
    }

    //Layout
    csmMap<csmString, csmFloat32> layout;
    _modelSetting->GetLayoutMap(layout);
    _modelMatrix->SetupFromLayout(layout);

    _model->SaveParameters();

    for (csmInt32 i = 0; i < _modelSetting->GetMotionGroupCount(); i++) {
        const csmChar *group = _modelSetting->GetMotionGroupName(i);
        PreloadMotionGroup(group);
    }

    _motionManager->StopAllMotions();

    _updating = false;
    _initialized = true;
    return true;
}

void LAppModel::PreloadMotionGroup(const csmChar *group) {
    const csmInt32 count = _modelSetting->GetMotionCount(group);

    for (csmInt32 i = 0; i < count; i++) {
        //ex) idle_0
        csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, i);
        csmString path = _modelSetting->GetMotionFileName(group, i);
        path = _modelHomeDir + path;
        CF_LOG_DEBUG("load motion: %s => [%s_%d] ", path.GetRawString(), group, i);

        csmByte *buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
        if (buffer) {
            auto *tmpMotion = dynamic_cast<CubismMotion *>(LoadMotion(buffer, size, name.GetRawString()));
            csmFloat32 fadeTime = _modelSetting->GetMotionFadeInTimeValue(group, i);
            if (fadeTime >= 0.0f) {
                tmpMotion->SetFadeInTime(fadeTime);
            }

            fadeTime = _modelSetting->GetMotionFadeOutTimeValue(group, i);
            if (fadeTime >= 0.0f) {
                tmpMotion->SetFadeOutTime(fadeTime);
            }
            tmpMotion->SetEffectIds(_eyeBlinkIds, _lipSyncIds);

            if (_motions[name] != nullptr) {
                ACubismMotion::Delete(_motions[name]);
            }
            _motions[name] = tmpMotion;

            DeleteBuffer(buffer, path.GetRawString());
        }
    }
}

void LAppModel::ReleaseMotionGroup(const csmChar *group) const {
    const csmInt32 count = _modelSetting->GetMotionCount(group);
    for (csmInt32 i = 0; i < count; i++) {
        csmString voice = _modelSetting->GetMotionSoundFileName(group, i);
        if (strcmp(voice.GetRawString(), "") != 0) {
            csmString path = voice;
            path = _modelHomeDir + path;
        }
    }
}

/**
* @brief すべてのモーションデータの解放
*
* すべてのモーションデータを解放する。
*/
void LAppModel::ReleaseMotions() {
    for (csmMap<csmString, ACubismMotion *>::const_iterator iter = _motions.Begin(); iter != _motions.End(); ++iter) {
        ACubismMotion::Delete(iter->Second);
    }

    _motions.Clear();
}

/**
* @brief すべての表情データの解放
*
* すべての表情データを解放する。
*/
void LAppModel::ReleaseExpressions() {
    for (csmMap<csmString, ACubismMotion *>::const_iterator iter = _expressions.Begin();
         iter != _expressions.End(); ++iter) {
        ACubismMotion::Delete(iter->Second);
    }

    _expressions.Clear();
}

void LAppModel::Update() {
    const csmFloat32 deltaTimeSeconds = LAppPal::GetDeltaTime();
    _userTimeSeconds += deltaTimeSeconds;

    _dragManager->Update(deltaTimeSeconds);
    _dragX = _dragManager->GetX();
    _dragY = _dragManager->GetY();

    // 根据运动更新参数的存在与否
    csmBool motionUpdated = false;

    //-----------------------------------------------------------------
    _model->LoadParameters(); // 前回セーブされた状態をロード 加载上次保存的状态
    if (_motionManager->IsFinished()) {
        // 如果没有可播放的动作，将从待机动作中随机播放一个。
        StartRandomMotion(MotionGroupIdle, PriorityIdle);
    } else {
        motionUpdated = _motionManager->UpdateMotion(_model, deltaTimeSeconds); // モーションを更新
    }
    _model->SaveParameters(); // 状態を保存
    //-----------------------------------------------------------------

    // 眨眼
    if (!motionUpdated) {
        if (_eyeBlink != nullptr) {
            // 当主要动作没有更新时。
            _eyeBlink->UpdateParameters(_model, deltaTimeSeconds); //眨眼
        }
    }

    if (_expressionManager != nullptr) {
        _expressionManager->UpdateMotion(_model, deltaTimeSeconds); // 根据表情更新参数（相对变化）。
    }

    //拖动更改
    //通过拖动调整脸部朝向
    _model->AddParameterValue(_idParamAngleX, _dragX , 30 * DRAG_SCALE); // 加上-30到30的值。
    _model->AddParameterValue(_idParamAngleY, _dragY , 30 * DRAG_SCALE);
    _model->AddParameterValue(_idParamAngleZ, _dragX , _dragY * -30 * DRAG_SCALE);

    //通过拖动调整身体方向
    _model->AddParameterValue(_idParamBodyAngleX, _dragX , 10 *DRAG_SCALE); // -10から10の値を加える

    //通过拖动调整眼睛方向
    _model->AddParameterValue(_idParamEyeBallX, _dragX,DRAG_SCALE); // -1から1の値を加える
    _model->AddParameterValue(_idParamEyeBallY, _dragY,DRAG_SCALE);

    // 呼吸等
    if (_breath != nullptr) {
        _breath->UpdateParameters(_model, deltaTimeSeconds);
    }

    // 物理模拟的设置
    if (_physics != nullptr) {
        _physics->Evaluate(_model, deltaTimeSeconds);
    }

    // 设置唇同步
    if (_lipSync) {
        // 如果要实时进行嘴唇同步，需要从系统中获取音量并以0〜1的范围输入值。
        // 这里是通过一个LAPPWAVFileHandler类的成员变量来获取和管理音量。
        csmFloat32 value = 0.0f;

        // 状態更新/RMS値取得
        _wavFileHandler.Update(deltaTimeSeconds);
        value = _wavFileHandler.GetRms();
        for (csmUint32 i = 0; i < _lipSyncIds.GetSize(); ++i) {
            _model->AddParameterValue(_lipSyncIds[i], value, 1000);
        }
    }

    // 姿势设置
    if (_pose != nullptr) {
        _pose->UpdateParameters(_model, deltaTimeSeconds);
    }

    _model->Update();

}

Csm::CubismMotionQueueEntryHandle
LAppModel::StartMotion(const Csm::csmChar *group, Csm::csmInt32 no, Csm::csmInt32 priority,
                       Csm::ACubismMotion::FinishedMotionCallback onFinishedMotionHandler,
                        std::shared_ptr<QByteArray> sound) {
    if (priority == PriorityForce) {
        _motionManager->SetReservePriority(priority);
    } else if (!_motionManager->ReserveMotion(priority)) {
        CF_LOG_DEBUG("can't start motion");
        return InvalidMotionQueueEntryHandleValue;
    }

    const csmString fileName = _modelSetting->GetMotionFileName(group, no);

    //ex) idle_0
    csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, no);
    CubismMotion *motion = dynamic_cast<CubismMotion *>(_motions[name.GetRawString()]);
    csmBool autoDelete = false;

    if (motion == nullptr) {
        csmString path = fileName;
        path = _modelHomeDir + path;

        csmByte *buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
        if (buffer) {
            motion = dynamic_cast<CubismMotion *>(LoadMotion(buffer, size, nullptr, onFinishedMotionHandler));
            csmFloat32 fadeTime = _modelSetting->GetMotionFadeInTimeValue(group, no);
            if (fadeTime >= 0.0f) {
                motion->SetFadeInTime(fadeTime);
            }

            fadeTime = _modelSetting->GetMotionFadeOutTimeValue(group, no);
            if (fadeTime >= 0.0f) {
                motion->SetFadeOutTime(fadeTime);
            }
            motion->SetEffectIds(_eyeBlinkIds, _lipSyncIds);
            autoDelete = true; // 終了時にメモリから削除

            DeleteBuffer(buffer, path.GetRawString());
        }
    } else {
        motion->SetFinishedMotionHandler(onFinishedMotionHandler);
    }

    //voice
    csmString voice = _modelSetting->GetMotionSoundFileName(group, no);
    if (strcmp(voice.GetRawString(), "") != 0 && resource_loader::get_instance().tts_enable_) {
        csmString path = voice;
        path = _modelHomeDir + path;
        _wavFileHandler.Start(path);
    } else if(sound && resource_loader::get_instance().tts_enable_)
    {
        _wavFileHandler.Start(sound);

    }
    CF_LOG_DEBUG("start motion: [%s_%d]", group, no);
    return _motionManager->StartMotionPriority(motion, autoDelete, priority);
}

CubismMotionQueueEntryHandle LAppModel::StartRandomMotion(const csmChar *group, csmInt32 priority,
                                                          ACubismMotion::FinishedMotionCallback onFinishedMotionHandler) {
    if (_modelSetting->GetMotionCount(group) == 0) {
        return InvalidMotionQueueEntryHandleValue;
    }
    csmInt32 no = rand() % _modelSetting->GetMotionCount(group);

    return StartMotion(group, no, priority, onFinishedMotionHandler, nullptr);
}

void LAppModel::DoDraw() {
    if (_model == nullptr) {
        return;
    }

    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->DrawModel();
}

void LAppModel::Draw(CubismMatrix44 &matrix) {
    if (_model == nullptr) {
        return;
    }

    matrix.MultiplyByMatrix(_modelMatrix);

    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->SetMvpMatrix(&matrix);

    DoDraw();
}

csmBool LAppModel::HitTest(const csmChar *hitAreaName, csmFloat32 x, csmFloat32 y) {
    // 透明时没有碰撞判定。。
    if (_opacity < 1) {
        return false;
    }
    const csmInt32 count = _modelSetting->GetHitAreasCount();
    for (csmInt32 i = 0; i < count; i++) {
        if (strcmp(_modelSetting->GetHitAreaName(i), hitAreaName) == 0) {
            const CubismIdHandle drawID = _modelSetting->GetHitAreaId(i);
            return IsHit(drawID, x, y);
        }
    }
    return false; // 如果不存在，则为false。
}

void LAppModel::SetExpression(const csmChar *expressionID) {
    ACubismMotion *motion = _expressions[expressionID];
    CF_LOG_DEBUG("expression: [%s]", expressionID);

    if (motion != nullptr) {
        _expressionManager->StartMotionPriority(motion, false, PriorityForce); // 将表情设置为强制优先级（优先级最高）
    } else {
        if (_debugMode) LAppPal::PrintLog("[APP]expression[%s] is nullptr ", expressionID);
    }
}

void LAppModel::SetRandomExpression() {
    if (_expressions.GetSize() == 0) {
        return;
    }

    csmInt32 no = rand() % _expressions.GetSize();
    csmMap<csmString, ACubismMotion *>::const_iterator map_ite;
    csmInt32 i = 0;
    for (map_ite = _expressions.Begin(); map_ite != _expressions.End(); map_ite++) {
        if (i == no) {
            csmString name = (*map_ite).First;
            SetExpression(name.GetRawString());
            return;
        }
        i++;
    }
}

void LAppModel::ReloadRenderer() {
    DeleteRenderer();

    CreateRenderer();

    SetupTextures();
}

void LAppModel::SetupTextures() {
    for (csmInt32 modelTextureNumber = 0; modelTextureNumber < _modelSetting->GetTextureCount(); modelTextureNumber++) {
        // テクスチャ名が空文字だった場合はロード・バインド処理をスキップ
        if (strcmp(_modelSetting->GetTextureFileName(modelTextureNumber), "") == 0) {
            continue;
        }

        //OpenGLのテクスチャユニットにテクスチャをロードする
        csmString texturePath = _modelSetting->GetTextureFileName(modelTextureNumber);
        texturePath = _modelHomeDir + texturePath;

        LAppTextureManager::TextureInfo *texture = LAppDelegate::GetInstance()->GetTextureManager()->CreateTextureFromPngFile(
                texturePath.GetRawString());
        if (texture != nullptr) {
            const csmInt32 glTextueNumber = texture->id;
            //OpenGL
            GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->BindTexture(modelTextureNumber, glTextueNumber);
        }
    }

#ifdef PREMULTIPLIED_ALPHA_ENABLE
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(true);
#else
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(false);
#endif

}

void LAppModel::MotionEventFired(const csmString &eventValue) {
    CubismLogInfo("%s is fired on LAppModel!!", eventValue.GetRawString());
}

Csm::Rendering::CubismOffscreenFrame_OpenGLES2 &LAppModel::GetRenderBuffer() {
    return _renderBuffer;
}

Csm::csmInt32 LAppModel::HitTest(Csm::csmFloat32 x, Csm::csmFloat32 y) {
    if (_opacity < 1) {
        return -1;
    }
    const csmInt32 count = _modelSetting->GetHitAreasCount();
    for (csmInt32 i = 0; i < count; i++) {
        const CubismIdHandle drawID = _modelSetting->GetHitAreaId(i);
        if (IsHit(drawID, x, y)) {
            return i;
        }
    }
    return -1;
}

Csm::csmBool LAppModel::StartRandomMotionOrExpression(Csm::csmInt32 hit_area_index,
                                                      Csm::ACubismMotion::FinishedMotionCallback onFinishedMotionHandler) {
    if (hit_area_index < 0 || hit_area_index >= _modelSetting->GetHitAreasCount()) {
        return false;
    }
    const csmChar *group = _modelSetting->GetHitAreaName(hit_area_index);
    if (_modelSetting->GetMotionCount(group) != 0) {
        csmInt32 no = rand() % _modelSetting->GetMotionCount(group);
        StartMotion(group, no, PriorityNormal, onFinishedMotionHandler, nullptr);
        return true;
    }
    else if (_modelSetting->GetExpressionCount() != 0) {
        SetRandomExpression();
        return true;
    }

    return InvalidMotionQueueEntryHandleValue;
}

Csm::csmBool LAppModel::ExpressionExists(const Csm::csmChar *expressionID) const {
    for (csmInt32 i = 0; i < _modelSetting->GetExpressionCount(); ++i) {
        if (strcmp(expressionID, _modelSetting->GetExpressionName(i)) == 0) {
            return true;
        }
    }
    return false;
}

Csm::csmBool LAppModel::MotionGroupExists(const Csm::csmChar *motion_group) const {
    for (csmInt32 i = 0; i < _modelSetting->GetMotionGroupCount(); ++i) {

        if (strcmp(motion_group, _modelSetting->GetMotionGroupName(i)) == 0) {
            return true;
        }
    }
    return false;
}
