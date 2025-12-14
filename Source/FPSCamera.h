#pragma once
#include "Maths.h"
#include"Window.h"

class FPSCamera {
public:
    Vec3 position;
    float yaw = 0.0f;      // 偏航角（左右）
    float pitch = 0.0f;    // 俯仰角（上下）

    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.002f;

    int lastMouseX = 0;
    int lastMouseY = 0;
    bool firstMouse = true;

    void update(float dt, Window* window) {
        // 获取当前鼠标位置
        int mouseX = window->getMouseInWindowX();
        int mouseY = window->getMouseInWindowY();

        // 第一次更新，初始化鼠标位置
        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        // 计算鼠标移动差值
        int deltaX = mouseX - lastMouseX;
        int deltaY = mouseY - lastMouseY;

        // 更新视角
        yaw += deltaX * mouseSensitivity;
        pitch -= deltaY * mouseSensitivity; // Y轴反转（FPS标准）

        // 限制俯仰角（避免翻转）
        pitch = clamp(pitch, -1.5f, 1.5f); // 约±86°

        // 保存当前鼠标位置
        lastMouseX = mouseX;
        lastMouseY = mouseY;

        // 键盘移动
        Vec3 moveDir(0, 0, 0);
        if (window->keyPressed('W')) moveDir.z += 1.0f;
        if (window->keyPressed('S')) moveDir.z -= 1.0f;
        if (window->keyPressed('A')) moveDir.x -= 1.0f;
        if (window->keyPressed('D')) moveDir.x += 1.0f;
        if (window->keyPressed(VK_SPACE)) moveDir.y += 1.0f;     // 跳跃
        if (window->keyPressed(VK_CONTROL)) moveDir.y -= 1.0f;   // 蹲下

        // 根据朝向计算移动方向
        if (moveDir.lengthSq() > 0.001f) {
            moveDir = moveDir.normalize();

            // 创建旋转矩阵
            Matrix rotY = Matrix::rotateY(yaw);
            Matrix rotX = Matrix::rotateX(pitch);
            Matrix totalRot = rotY * rotX;
			
            // 只使用Y轴旋转计算水平移动
			Vec3 forward = rotY.mulVec(Vec3(0, 0, 1));
            //Vec3 right = Vec3(1, 0, 0).mulVec(rotY);
			Vec3 right = rotY.mulVec(Vec3(1, 0, 0));
            Vec3 up = Vec3(0, 1, 0);

            // 计算实际移动向量
            Vec3 actualMove = (forward * moveDir.z) +
                (right * moveDir.x) +
                (up * moveDir.y);

            // 更新位置
            position += actualMove * moveSpeed * dt;
        }
    }

    Matrix getViewMatrix() {
        // 计算方向向量
        Matrix rotY = Matrix::rotateY(yaw);
        Matrix rotX = Matrix::rotateX(pitch);
        Matrix totalRot = rotY * rotX;

        //Vec3 forward = Vec3(0, 0, 1).mulVec(totalRot);
        //Vec3 up = Vec3(0, 1, 0).mulVec(rotX); // 保持正确的up向量
		Vec3 forward = totalRot.mulVec(Vec3(0, 0, 1));
		//Vec3 up = totalRot.mulVec(Vec3(0, 1, 0));
		Vec3 up = rotX.mulVec(Vec3(0, 1, 0));

        // 计算目标点
        Vec3 target = position + forward;

        // 返回视图矩阵
        return Matrix::lookAt(position, target, up);
    }

    Vec3 getForwardVector() {
        Matrix rotY = Matrix::rotateY(yaw);
        Matrix rotX = Matrix::rotateX(pitch);
        Matrix totalRot = rotY * rotX;
		return totalRot.mulVec(Vec3(0, 0, 1));
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