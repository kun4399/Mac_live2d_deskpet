﻿/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismRenderer.hpp"
#include "CubismFramework.hpp"
#include "Model/CubismModel.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

void CubismRenderer::Delete(CubismRenderer* renderer)
{
    CSM_DELETE_SELF(CubismRenderer, renderer);
}

CubismRenderer::CubismRenderer()
    : _isCulling(false)
    , _isPremultipliedAlpha(false)
    , _anisotropy(0.0f)
    , _model(NULL)
    , _useHighPrecisionMask(false)
{
    //単位行列に初期化
    _mvpMatrix4x4.LoadIdentity();
}

CubismRenderer::~CubismRenderer()
{}

void CubismRenderer::Initialize(Framework::CubismModel* model)
{
    Initialize(model, 1);
}

void CubismRenderer::Initialize(Framework::CubismModel* model, csmInt32 maskBufferCount)
{
    _model = model;
}

void CubismRenderer::DrawModel()
{
    if (GetModel() == NULL) return;

    /**
     * 在绘制DoDrawModel之前和绘制之后，请调用以下函数：。。
     * ・SaveProfile();
     * ・RestoreProfile();
     * 这是通过保存和恢复渲染器绘图设置
     * 用于还原到模型绘制之前的状态的过程。
     */

    SaveProfile();

    DoDrawModel();

    RestoreProfile();
}

void CubismRenderer::SetMvpMatrix(CubismMatrix44* matrix4x4)
{
    _mvpMatrix4x4.SetMatrix(matrix4x4->GetArray());
}

CubismMatrix44 CubismRenderer::GetMvpMatrix() const
{
    return _mvpMatrix4x4;
}

void CubismRenderer::SetModelColor(csmFloat32 red, csmFloat32 green, csmFloat32 blue, csmFloat32 alpha)
{
    if (red < 0.0f) red = 0.0f;
    else if (red > 1.0f) red = 1.0f;

    if (green < 0.0f) green = 0.0f;
    else if (green > 1.0f) green = 1.0f;

    if (blue < 0.0f) blue = 0.0f;
    else if (blue > 1.0f) blue = 1.0f;

    if (alpha < 0.0f) alpha = 0.0f;
    else if (alpha > 1.0f) alpha = 1.0f;

    _modelColor.R = red;
    _modelColor.G = green;
    _modelColor.B = blue;
    _modelColor.A = alpha;
}

CubismRenderer::CubismTextureColor CubismRenderer::GetModelColor() const
{
    return  _modelColor;
}

void CubismRenderer::IsPremultipliedAlpha(csmBool enable)
{
    _isPremultipliedAlpha = enable;
}

csmBool CubismRenderer::IsPremultipliedAlpha() const
{
    return _isPremultipliedAlpha;
}

void CubismRenderer::IsCulling(csmBool culling)
{
    _isCulling = culling;
}

csmBool CubismRenderer::IsCulling() const
{
    return  _isCulling;
}

void CubismRenderer::SetAnisotropy(csmFloat32 n)
{
    _anisotropy = n;
}

csmFloat32 CubismRenderer::GetAnisotropy() const
{
    return _anisotropy;
}

CubismModel* CubismRenderer::GetModel() const
{
    return _model;
}

void CubismRenderer::UseHighPrecisionMask(csmBool high)
{
    _useHighPrecisionMask = high;
}

csmBool CubismRenderer::IsUsingHighPrecisionMask()
{
    return _useHighPrecisionMask;
}

}}}}

//------------ LIVE2D NAMESPACE ------------
