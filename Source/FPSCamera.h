#pragma once
#include "Maths.h"
#include"Window.h"

class FPSCamera {
public:
    Vec3 position;
    float yaw = 0.0f;     
    float pitch = 0.0f;    

    float moveSpeed = 4.0f;
    float mouseSensitivity = 0.002f;

    int lastMouseX = 0;
    int lastMouseY = 0;
    bool firstMouse = true;

    void update(float dt, Window* window) {
        // 1. Get Window Center
        RECT rect;
        GetClientRect(window->hwnd, &rect);
        int centerX = rect.right / 2;
        int centerY = rect.bottom / 2;

        // 2. Current mouse position (window coordinates)
        int mouseX = window->getMouseInWindowX();
        int mouseY = window->getMouseInWindowY();

        // 3. Offset = relative to center
        int deltaX = mouseX - centerX;
        int deltaY = mouseY - centerY;

        // 4. Update angle
        yaw -= deltaX * mouseSensitivity;
       // pitch += deltaY * mouseSensitivity;
        pitch -= deltaY * mouseSensitivity;



        // 5. Limit pitch ,prevent flipping
        pitch = clamp(pitch, -1.5f, 1.5f);

        // 6reset the mouse cursor to the center 
        window->setMousePositionToCenter();


        // 键盘移动
        Vec3 moveDir(0, 0, 0);
        if (window->keyPressed('W')) moveDir.z += 1.0f;
        if (window->keyPressed('S')) moveDir.z -= 1.0f;
        if (window->keyPressed('A')) moveDir.x += 1.0f;
        if (window->keyPressed('D')) moveDir.x -= 1.0f;
        //if (window->keyPressed(VK_SPACE)) moveDir.y += 1.0f;     // JUMP
		//if (window->keyPressed(VK_CONTROL)) moveDir.y -= 1.0f;   // CROUCH


           if (moveDir.lengthSq() > 0.001f) {
                moveDir = moveDir.normalize();

                // Calculate the local coordinate system axes of the camera
                Matrix rotY = Matrix::rotateY(yaw);

                Vec3 forward = rotY.mulVec(Vec3(0, 0, -1));  //
                Vec3 right = rotY.mulVec(Vec3(1, 0, 0));
                Vec3 up = Vec3(0, 1, 0);

                Vec3 actualMove = (forward * moveDir.z) + (right * moveDir.x) + (up * moveDir.y);

                position += actualMove * moveSpeed * dt;
            }
        
    }

    //Matrix getViewMatrix() {
    //    
    //    Matrix rotY = Matrix::rotateY(yaw);
    //    Matrix rotX = Matrix::rotateX(pitch);
    //    Matrix totalRot = rotY * rotX;

    //    Vec3 forward = totalRot.mulVec(Vec3(0, 0, -1));
    //    //Vec3 up = rotX.mulVec(Vec3(0, 1, 0));
    //    Vec3 up = Vec3(0, 1, 0);

    //    Vec3 target = position + forward;

    //    return Matrix::lookAt(position, target, up);
    //}


    Matrix getViewMatrix()
    {
     
        float cosPitch = cosf(pitch);
        float sinPitch = sinf(pitch);
        float cosYaw = cosf(yaw);
        float sinYaw = sinf(yaw);

        
        Vec3 forward(
            sinYaw * cosPitch,
            sinPitch,
            -cosYaw * cosPitch
        );
        forward = forward.normalize();

        // Use worldUp to dynamically calculate right/up
        Vec3 worldUp(0, 1, 0);
        Vec3 right = Cross(worldUp, forward).normalize();
        Vec3 up = Cross(forward, right).normalize();

        Vec3 target = position + forward;
        return Matrix::lookAt(position, target, up);
    }


    
    Matrix getViewMatrix2() {

        Matrix rotation = Matrix::rotateX(-pitch) * Matrix::rotateY(-yaw);
        Matrix translation = Matrix::translation(-position);

  
        return rotation * translation;
    }

    Matrix getViewMatrix3() {
        
        Matrix rotY = Matrix::rotateY(yaw);
        Matrix rotX = Matrix::rotateX(pitch);
        Matrix totalRot = rotY * rotX;

        // The camera's forward direction (looking -Z)
        Vec3 forward = totalRot.mulVec(Vec3(0, 0, -1));

        // Calculate the correct up vector
        Vec3 worldUp = Vec3(0, 1, 0);

        Vec3 target = position + forward;
        return Matrix::lookAt(position, target, worldUp);
    }


    Matrix getViewMatrix4() {
        
        Matrix view;

        float cosPitch = cosf(pitch);
        float sinPitch = sinf(pitch);
        float cosYaw = cosf(yaw);
        float sinYaw = sinf(yaw);

        // Constructing the camera coordinate system
        Vec3 forward = Vec3(sinYaw * cosPitch, sinPitch, cosYaw * cosPitch);
        Vec3 right = Vec3(cosYaw, 0, -sinYaw);
        Vec3 up = Cross(right, forward).normalize();

        // Construct view matrix
        view.m[0] = right.x; view.m[4] = right.y; view.m[8] = right.z;
        view.m[1] = up.x;    view.m[5] = up.y;    view.m[9] = up.z;
        view.m[2] = -forward.x; view.m[6] = -forward.y; view.m[10] = -forward.z;

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

    //void resetMouse(Window* window) {
    //    firstMouse = true;
    //    RECT rect;
    //    GetClientRect(window->hwnd, &rect);
    //    POINT center = { rect.right / 2, rect.bottom / 2 };
    //    ClientToScreen(window->hwnd, &center);
    //    SetCursorPos(center.x, center.y);
    //}
    void reset(Vec3 newPosition = Vec3(0, 1.7f, 5), float newYaw = 0, float newPitch = 0) {
        position = newPosition;
        yaw = newYaw;
        pitch = newPitch;

        firstMouse = true;
    }
};