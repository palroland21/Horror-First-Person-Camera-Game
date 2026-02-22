#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        //TODO  --> DONE
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        // right <=> perpendicular cu Front si Up (prod. vectorial)
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection)); 
        
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        //TODO --> target = cameraPos + FrontDiresction
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        // Calculez un vector de directie care sa ignore axa Y ca sa putem merge doar in fata, nu sa zburam in sus
        glm::vec3 miscareInFata = glm::normalize(glm::vec3(cameraFrontDirection.x, 0.0f, cameraFrontDirection.z));

        switch (direction) {
        case MOVE_FORWARD:
            cameraPosition += miscareInFata * speed;  // miscareInFata
            break;

        case MOVE_BACKWARD:
            cameraPosition -= miscareInFata * speed;  // miscareInFata
            break;

        case MOVE_RIGHT:
            cameraPosition += cameraRightDirection * speed;
            break;

        case MOVE_LEFT:
            cameraPosition -= cameraRightDirection * speed;
            break;
        }

    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        this->cameraFrontDirection = glm::normalize(front);

        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        // Dreapta = Prod vect(Front, WorldUp)
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, worldUp));
        // Sus = prod vect(right, front)
        this->cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }
}
