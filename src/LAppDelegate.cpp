﻿/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"

using namespace Csm;
using namespace std;
using namespace LAppDefine;

namespace {
    LAppDelegate *s_instance = nullptr;
}

LAppDelegate *LAppDelegate::GetInstance() {
    if (s_instance == nullptr) {
        s_instance = new LAppDelegate();
    }

    return s_instance;
}

void LAppDelegate::ReleaseInstance() {
    if (s_instance != nullptr) {
        delete s_instance;
    }

    s_instance = NULL;
}

bool LAppDelegate::Initialize(GLWidget *window) {
    if (DebugLogEnable) {
        LAppPal::PrintLog("START");
    }


    if (glewInit() != GLEW_OK) {
        if (DebugLogEnable) {
            LAppPal::PrintLog("Can't initilize glew.");
        }
        return GL_FALSE;
    }

    //テクスチャサンプリング設定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //透過設定
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //コールバック関数の登録
    // ウィンドウサイズ記憶
    _windowWidth = window->width();
    _windowHeight = window->height();
    _window = window;
    //AppViewの初期化
    _view->Initialize();

    // Cubism SDK の初期化
    InitializeCubism();
    glViewport(0, 0, _windowWidth, _windowHeight);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return GL_TRUE;
}

void LAppDelegate::Release() {
    // Windowの削除
    glfwTerminate();

    delete _textureManager;
    delete _view;

    // リソースを解放
    LAppLive2DManager::ReleaseInstance();

    //Cubism SDK の解放
    CubismFramework::Dispose();
}


void LAppDelegate::resize(int width, int height) {
    if ((_windowWidth != width || _windowHeight != height) && width > 0 && height > 0) {
        //AppViewの初期化
        _view->Initialize();
        // スプライトサイズを再設定
        //_view->ResizeSprite();
        // サイズを保存しておく
        _windowWidth = width;
        _windowHeight = height;

        // ビューポート変更
        glViewport(0, 0, width, height);
    }
}

void LAppDelegate::update() {
    // 時間更新
    LAppPal::UpdateTime();

    // 画面の初期化
    glClearColor(0.0f, 0.0f, 0.0f, _alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearDepth(1.0);


    //描画更新
    _view->Render();
}

#if 0
void LAppDelegate::Run()
{
    //メインループ
    while (glfwWindowShouldClose(_window) == GL_FALSE && !_isEnd)
    {
        int width, height;
        glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
        if( (_windowWidth!=width || _windowHeight!=height) && width>0 && height>0)
        {
            //AppViewの初期化
            _view->Initialize();
            // スプライトサイズを再設定
            _view->ResizeSprite();
            // サイズを保存しておく
            _windowWidth = width;
            _windowHeight = height;

            // ビューポート変更
            glViewport(0, 0, width, height);
        }

        // 時間更新
        LAppPal::UpdateTime();

        // 画面の初期化
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearDepth(1.0);

        //描画更新
        _view->Render();

        // バッファの入れ替え
        glfwSwapBuffers(_window);

        // Poll for and process events
    }

    Release();

    LAppDelegate::ReleaseInstance();
}
#endif

LAppDelegate::LAppDelegate() :
        _cubismOption(),
        _window(nullptr),
        _captured(false),
        _mouseX(0.0f),
        _mouseY(0.0f),
        _isEnd(false),
        _windowWidth(0),
        _windowHeight(0) {
    _view = new LAppView();
    _textureManager = new LAppTextureManager();
}

LAppDelegate::~LAppDelegate() = default;

void LAppDelegate::InitializeCubism() {
    //setup cubism
    _cubismOption.LogFunction = LAppPal::PrintMessage;
    _cubismOption.LoggingLevel = LAppDefine::CubismLoggingLevel;
    Csm::CubismFramework::StartUp(&_cubismAllocator, &_cubismOption);

    //Initialize cubism
    CubismFramework::Initialize();

    //load model
    LAppLive2DManager::GetInstance();

    //default proj
    CubismMatrix44 projection;

    LAppPal::UpdateTime();

    _view->InitializeSprite();
}

#if 0
void LAppDelegate::OnMouseCallBack(int button, int action, int modify)
{
    if (_view == NULL)
    {
        return;
    }
    if (GLFW_MOUSE_BUTTON_LEFT != button)
    {
        return;
    }

    if (GLFW_PRESS == action)
    {
        _captured = true;
        _view->OnTouchesBegan(_mouseX, _mouseY);
    }
    else if (GLFW_RELEASE == action)
    {
        if (_captured)
        {
            _captured = false;
            _view->OnTouchesEnded(_mouseX, _mouseY);
        }
    }
}

void LAppDelegate::OnMouseCallBack(double x, double y)
{
    _mouseX = static_cast<float>(x);
    _mouseY = static_cast<float>(y);

    if (!_captured)
    {
        return;
    }
    if (_view == NULL)
    {
        return;
    }

    _view->OnTouchesMoved(_mouseX, _mouseY);
}
#endif

void LAppDelegate::mousePressEvent(int x, int y) {
//    qDebug("LAppDelegate::mousePressEvent");
    if (_view == nullptr) {
        return;
    }
    _captured = true;
    _view->OnTouchesBegan((float)x, (float)y);
}

void LAppDelegate::mouseReleaseEvent(int x, int y) {
    if (_view == nullptr) {
        return;
    }
    if (_captured) {
        _captured = false;
        _view->OnTouchesEnded((float) x, (float) y);
    }
}

void LAppDelegate::mouseMoveEvent(int x, int y) {
//    _mouseX = static_cast<float>(x);
//    _mouseY = static_cast<float>(y);
//    qDebug("captured:%d",_captured);
    if (!_captured) {
        return;
    }
    if (_view == nullptr) {
        return;
    }
    _view->OnTouchesMoved(x, y);
}

GLuint LAppDelegate::CreateShader() {
    //バーテックスシェーダのコンパイル
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    const char *vertexShader =
            "#version 120\n"
            "attribute vec3 position;"
            "attribute vec2 uv;"
            "varying vec2 vuv;"
            "void main(void){"
            "    gl_Position = vec4(position, 1.0);"
            "    vuv = uv;"
            "}";
    glShaderSource(vertexShaderId, 1, &vertexShader, NULL);
    glCompileShader(vertexShaderId);
    if (!CheckShader(vertexShaderId)) {
        return 0;
    }

    //フラグメントシェーダのコンパイル
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragmentShader =
            "#version 120\n"
            "varying vec2 vuv;"
            "uniform sampler2D texture;"
            "uniform vec4 baseColor;"
            "void main(void){"
            "    gl_FragColor = texture2D(texture, vuv) * baseColor;"
            "}";
    glShaderSource(fragmentShaderId, 1, &fragmentShader, NULL);
    glCompileShader(fragmentShaderId);
    if (!CheckShader(fragmentShaderId)) {
        return 0;
    }

    //プログラムオブジェクトの作成
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    // リンク
    glLinkProgram(programId);

    glUseProgram(programId);

    return programId;
}

bool LAppDelegate::CheckShader(GLuint shaderId) {
    GLint status;
    GLint logLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = reinterpret_cast<GLchar *>(CSM_MALLOC(logLength));
        glGetShaderInfoLog(shaderId, logLength, &logLength, log);
        CubismLogError("Shader compile log: %s", log);
        CSM_FREE(log);
    }

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glDeleteShader(shaderId);
        return false;
    }

    return true;
}
