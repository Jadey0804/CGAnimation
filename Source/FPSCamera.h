#pragma once
#include "Maths.h"
#include"Window.h"

class FPSCamera {
public:
    Vec3 position;
    float yaw = 0.0f;     
    float pitch = 0.0f;    

    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.002f;

    int lastMouseX = 0;
    int lastMouseY = 0;
    bool firstMouse = true;

    void update(float dt, Window* window) {
        // 1. 获取窗口中心
        RECT rect;
        GetClientRect(window->hwnd, &rect);
        int centerX = rect.right / 2;
        int centerY = rect.bottom / 2;

        // 2. 当前鼠标位置（窗口坐标）
        int mouseX = window->getMouseInWindowX();
        int mouseY = window->getMouseInWindowY();

        // 3. 偏移 = 相对中心
        int deltaX = mouseX - centerX;
        int deltaY = mouseY - centerY;

        // 4. 更新角度
        yaw -= deltaX * mouseSensitivity;
       // pitch += deltaY * mouseSensitivity;
        pitch -= deltaY * mouseSensitivity;



        // 5. 限制 pitch（防止翻转）
        pitch = clamp(pitch, -1.5f, 1.5f);

        // 6. **关键：立刻把鼠标重置回中心**
        window->setMousePositionToCenter();


        // 键盘移动
        Vec3 moveDir(0, 0, 0);
        if (window->keyPressed('W')) moveDir.z += 1.0f;
        if (window->keyPressed('S')) moveDir.z -= 1.0f;
        if (window->keyPressed('A')) moveDir.x += 1.0f;
        if (window->keyPressed('D')) moveDir.x -= 1.0f;
        if (window->keyPressed(VK_SPACE)) moveDir.y += 1.0f;     // 跳跃
        if (window->keyPressed(VK_CONTROL)) moveDir.y -= 1.0f;   // 蹲下


           if (moveDir.lengthSq() > 0.001f) {
                moveDir = moveDir.normalize();

                // 计算相机的局部坐标系轴
                Matrix rotY = Matrix::rotateY(yaw);

                // 注意：在视图空间中，相机的：
                // 前方向 = 局部 -Z 轴
                // 右方向 = 局部 +X 轴  
                // 上方向 = 局部 +Y 轴

                Vec3 forward = rotY.mulVec(Vec3(0, 0, -1));  //
                Vec3 right = rotY.mulVec(Vec3(1, 0, 0));
                Vec3 up = Vec3(0, 1, 0);

                Vec3 actualMove = (forward * moveDir.z) + (right * moveDir.x) + (up * moveDir.y);

                position += actualMove * moveSpeed * dt;
            }
        
    }

    //Matrix getViewMatrix() {
    //    // 计算方向向量
    //    Matrix rotY = Matrix::rotateY(yaw);//偏航角（左右转头）绕Y轴
    //    Matrix rotX = Matrix::rotateX(pitch);//俯仰角（上下转头）
    //    Matrix totalRot = rotY * rotX;//注意顺序，先绕X轴旋转，再绕Y轴

    //    Vec3 forward = totalRot.mulVec(Vec3(0, 0, -1));
    //    //Vec3 up = rotX.mulVec(Vec3(0, 1, 0));
    //    Vec3 up = Vec3(0, 1, 0);

    //    // 计算目标点
    //    Vec3 target = position + forward;

    //    // 返回视图矩阵
    //    return Matrix::lookAt(position, target, up);
    //}


    Matrix getViewMatrix()
    {
        // 1) 用 yaw/pitch 直接算 forward（避免旋转顺序/轴的问题）
        float cosPitch = cosf(pitch);
        float sinPitch = sinf(pitch);
        float cosYaw = cosf(yaw);
        float sinYaw = sinf(yaw);

        // 这里的 forward 让 yaw=0 时朝向 -Z（和你原来一致）
        Vec3 forward(
            sinYaw * cosPitch,
            sinPitch,
            -cosYaw * cosPitch
        );
        forward = forward.normalize();

        // 2) 用 worldUp 动态算 right/up，避免“看向背后时上下翻转”
        Vec3 worldUp(0, 1, 0);
        Vec3 right = Cross(worldUp, forward).normalize();
        Vec3 up = Cross(forward, right).normalize();

        Vec3 target = position + forward;
        return Matrix::lookAt(position, target, up);
    }


    Matrix getViewMatrix2() {
        // 方法1：直接构建视图矩阵（更可靠）
        // 视图矩阵 = 旋转 * 平移（相机变换的逆）

        // 先绕Y轴旋转（yaw），再绕X轴旋转（pitch）
        Matrix rotation = Matrix::rotateX(-pitch) * Matrix::rotateY(-yaw);
        Matrix translation = Matrix::translation(-position);

        // 视图矩阵 = 旋转 * 平移
        return rotation * translation;
    }

    Matrix getViewMatrix3() {
        // 计算相机的朝向向量
        Matrix rotY = Matrix::rotateY(yaw);
        Matrix rotX = Matrix::rotateX(pitch);
        Matrix totalRot = rotY * rotX;

        // 相机的向前方向（看向 -Z）
        Vec3 forward = totalRot.mulVec(Vec3(0, 0, -1));

        // 计算正确的up向量（考虑pitch的影响）
        Vec3 worldUp = Vec3(0, 1, 0);

        // 防止forward和worldUp平行
        if (fabsf(Dot(forward, worldUp)) > 0.9999f) {
            worldUp = Vec3(0, 0, 1);  // 如果几乎平行，使用备用up
        }

        Vec3 target = position + forward;
        return Matrix::lookAt(position, target, worldUp);
    }


    Matrix getViewMatrix4() {
        // 直接构建视图矩阵
        Matrix view;

        float cosPitch = cosf(pitch);
        float sinPitch = sinf(pitch);
        float cosYaw = cosf(yaw);
        float sinYaw = sinf(yaw);

        // 构建相机坐标系
        Vec3 forward = Vec3(sinYaw * cosPitch, sinPitch, cosYaw * cosPitch);
        Vec3 right = Vec3(cosYaw, 0, -sinYaw);
        Vec3 up = Cross(right, forward).normalize();

        // 构建视图矩阵
        view.m[0] = right.x; view.m[4] = right.y; view.m[8] = right.z;
        view.m[1] = up.x;    view.m[5] = up.y;    view.m[9] = up.z;
        view.m[2] = -forward.x; view.m[6] = -forward.y; view.m[10] = -forward.z;

        // 平移部分
        view.m[12] = -Dot(position, right);
        view.m[13] = -Dot(position, up);
        view.m[14] = Dot(position, forward);
        view.m[15] = 1.0f;

        return view;
    }









    Vec3 getForwardVector() {
        Matrix rotY = Matrix::rotateY(yaw);
        Matrix rotX = Matrix::rotateX(pitch);
        Matrix totalRot = rotY * rotX;
		return totalRot.mulVec(Vec3(0, 0, -1));
    }

    Vec3 getRightVector() {
        Matrix rotY = Matrix::rotateY(yaw);
		return rotY.mulVec(Vec3(1, 0, 0));
    }

    void resetMouse(Window* window) {
        firstMouse = true;
        // 重置鼠标到窗口中心
        RECT rect;
        GetClientRect(window->hwnd, &rect);
        POINT center = { rect.right / 2, rect.bottom / 2 };
        ClientToScreen(window->hwnd, &center);
        SetCursorPos(center.x, center.y);
    }
    void reset(Vec3 newPosition = Vec3(0, 1.7f, 5), float newYaw = 0, float newPitch = 0) {
        position = newPosition;
        yaw = newYaw;
        pitch = newPitch;

        firstMouse = true;
    }
};