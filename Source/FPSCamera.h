#pragma once
#include "Maths.h"
#include "Window.h"

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
        if (window->keyPressed('W')) moveDir.z -= 1.0f;
        if (window->keyPressed('S')) moveDir.z += 1.0f;
        if (window->keyPressed('A')) moveDir.x -= 1.0f;
        if (window->keyPressed('D')) moveDir.x += 1.0f;
        if (window->keyPressed(VK_SPACE)) moveDir.y += 1.0f;     // 跳跃
        if (window->keyPressed(VK_CONTROL)) moveDir.y -= 1.0f;   // 蹲下

  
   //         // 创建旋转矩阵
   //         Matrix rotY = Matrix::rotateY(yaw);
   //         Matrix rotX = Matrix::rotateX(pitch);
   //         Matrix totalRot = rotY * rotX;
			//
   //         // 只使用Y轴旋转计算水平移动
			//Vec3 forward = rotY.mulVec(Vec3(0, 0, 1));
   //         //Vec3 right = Vec3(1, 0, 0).mulVec(rotY);
			//Vec3 right = rotY.mulVec(Vec3(1, 0, 0));
   //         Vec3 up = Vec3(0, 1, 0);

   //         // 计算实际移动向量
   //         Vec3 actualMove = (forward * moveDir.z) +
   //             (right * moveDir.x) +
   //             (up * moveDir.y);

   //         // 更新位置
   //         position += actualMove * moveSpeed * dt;
   // 
   // 
            // 键盘移动 - 使用相机的局部坐标系
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

                Vec3 actualMove = (forward * moveDir.z) +
                    (right * moveDir.x) +
                    (up * moveDir.y);

                position += actualMove * moveSpeed * dt;
            }
        
    }

    Matrix getViewMatrix() {
        // 计算方向向量
        Matrix rotY = Matrix::rotateY(yaw);//偏航角（左右转头）绕Y轴
		Matrix rotX = Matrix::rotateX(pitch);//俯仰角（上下转头）
		Matrix totalRot = rotY * rotX;//注意顺序，先绕X轴旋转，再绕Y轴旋转，FPS相机的旋转顺序/如果是rotX * rotY,先上下看，再左右看。类似飞机控制，会出现翻滚
 
		Vec3 forward = totalRot.mulVec(Vec3(0, 0, -1));
		//Vec3 up = rotX.mulVec(Vec3(0, 1, 0));
		Vec3 up = Vec3(0, 1, 0);

        // 计算目标点
        Vec3 target = position + forward;

        // 返回视图矩阵
        return Matrix::lookAt(position, target, up);






    //        // 方法1：直接构建视图矩阵（推荐）
    //// 视图矩阵 = T(-position) * R_yaw * R_pitch
    //// 注意：视图矩阵是相机变换的逆矩阵

    //    Matrix translation = Matrix::translation(-position);  // 平移到相机位置

    //    // 重要：先绕X轴旋转（Pitch），再绕Y轴旋转（Yaw）
    //    // 这是相机自身的旋转顺序
    //    Matrix rotation = Matrix::rotateX(-pitch) * Matrix::rotateY(-yaw);

    //    // 视图矩阵 = 旋转 * 平移
    //    return rotation * translation;
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