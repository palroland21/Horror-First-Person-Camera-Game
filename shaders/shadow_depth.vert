#version 410 core

layout(location=0) in vec3 vPosition;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main()
{
     // vPosition = pozitia unui vertex in model
	 // model * vec4(..) il muta in WORLD SPACE
	 // lightSpaceMatrix * .. il muta in LIGHT VIEW
    gl_Position = lightSpaceMatrix * model * vec4(vPosition, 1.0); // fiecare vertex e tratat ca si cum ar fi o camera ( camera luminii )
}
