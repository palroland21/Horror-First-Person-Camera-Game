#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec2 TexCoords;
out vec3 FragPosWorld;
out vec3 NormalWorld;
flat out vec3 NormalWorldFlat;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// pentru umbre
uniform mat4 lightSpaceMatrix;

void main()
{
    TexCoords = vTexCoords; // UV trec mai departe 

    vec4 worldPos = model * vec4(vPosition, 1.0);
    FragPosWorld = worldPos.xyz;

	// normala in world space
    NormalWorld = mat3(transpose(inverse(model))) * vNormal;
    NormalWorldFlat = NormalWorld; // aceeasi valoare, dar FLAT (pe tot triunghiul fragment sahderul va primi o singura normala)

	// pozitia in light space pt shadow map
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos; // pozitia finala pe ecran a vertexului curent din Mesh, dupa ce a trecut prin: model -> world -> view -> projection -> clip space
}
