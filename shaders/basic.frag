#version 410 core

in vec2 TexCoords;
in vec3 FragPosWorld;
in vec3 NormalWorld;
flat in vec3 NormalWorldFlat;
in vec4 FragPosLightSpace;

out vec4 fColor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// camera/view
uniform mat4 view;

// lanterna (spotlight)
uniform vec3 flashPos;      // world
uniform vec3 flashDir;      // world
uniform vec3 flashColor;
uniform float unghiInterior;   // cos(inner)
uniform float unghiExterior;   // cos(outer)
uniform float constantAttenuation;
uniform float linearAttenuation;
uniform float quadraticAttenuation;

// lampa (point light)
uniform int lampOn;
uniform vec3 lampPos;       // world
uniform vec3 lampColor;
uniform float lampConst;
uniform float lampLinear;
uniform float lampQuad;

// umbre
uniform sampler2D shadowMap;

// material - un minim de lumina sa nu fie negru
float ambientStrength = 0.01;
float specularStrength = 0.5; 

uniform int uShadingMode; // 0 = smooth, 1 = flat

float ShadowCalc(vec4 fragPosLightSpace, vec3 normalW, vec3 lightDirW)
{
    // proiectia din light space in coordonate de textura (UV + depth)
    vec3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w; // NDC [-1..1]
    proj = proj * 0.5 + 0.5;                                 // [0..1]

	// daca e in afara frustrumului luminii sau in spatele far plane => fara umbra
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 0.0;
    if (proj.z > 1.0) return 0.0;

    float currentDepth = proj.z; // din perspectiva luminii

    // cand comparam adancimile din cauza FLOAT, fragmentele nu sunt chiar exacte => apar pete pe suprafete iluminate
    float lightFacing = max(dot(normalize(normalW), normalize(lightDirW)), 0.0); // daca suprafata e aproape perpendiculara pe lumina => am nevoie de bias mai mare
    float bias = max(0.0003 * (1.0 - lightFacing), 0.00005); // creste BIAS cand LIGHTFACING scade, dar seteaza un minimum ca si prag

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0)); // (1/1024, 1/1024) pt ca rezolutia texturii e 1024x1024
    float shadow = 0.0;

    // PCF 3x3  
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, proj.xy + vec2(x, y) * texelSize).r; // valoarea .r (rosu) contine adancimea; vec2(x,y) = coordonatele vecinilor, proj.xy = coordonate a carui vecin le iau
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }

    return shadow / 9.0; // obtine o valoare intre [0,1], adica cat de soft e umbra
}

// Laterna din mana
void computeFlashLight(vec3 normalW, out vec3 amb, out vec3 diff, out vec3 spec, out vec3 LdirW)
{
    vec3 N = normalize(normalW);

    // vector de la fragment spre lumina (world)
    vec3 L = normalize(flashPos - FragPosWorld);
    LdirW = L;

    // spotlight (flashDir e directia in care bate lanterna)
    vec3 spotDir = normalize(flashDir);
    float x = dot(normalize(FragPosWorld - flashPos), spotDir); // cosinului dintre directia spotului si directia spre fragment
    float y = unghiInterior - unghiExterior; // direct cos(inner) si cos(outer) venie din C
    float intensity = clamp((x - unghiExterior) / y, 0.0, 1.0); // intensitate 0 => in afara conului ; intesitate = 1 (in interior)

    // atenuare
    float dist = length(flashPos - FragPosWorld); 
    float attenuation = 1.0 / (constantAttenuation + linearAttenuation * dist + quadraticAttenuation * dist * dist);

    // Phong in eye-space pt viewDir
    vec3 fragPosEye = vec3(view * vec4(FragPosWorld, 1.0));
    vec3 viewDir = normalize(-fragPosEye); // eye-space camera e la (0,0,0) => fragment -> camera devine -fragPosEye

        // transform pozitia + directia lanternei in eye-space
    vec3 lightPosEye = vec3(view * vec4(flashPos, 1.0));
    vec3 Leye = normalize(lightPosEye - fragPosEye); 

    vec3 normalEye = normalize(mat3(view) * N);

    float diffCoeff = max(dot(normalEye, Leye), 0.0);
    vec3 reflectDir = reflect(-Leye, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    amb  = ambientStrength * flashColor * attenuation;
    diff = diffCoeff * flashColor * intensity * attenuation;
    spec = specularStrength * specCoeff * flashColor * intensity * attenuation;
}

// Lampa statica din harta
void computeLampPointLight(vec3 normalW, out vec3 diff, out vec3 spec)
{
    vec3 N = normalize(normalW);

    // transform pozitiile in eye-space ca sa calcul Phong
    vec3 lampPosEye = vec3(view * vec4(lampPos, 1.0));
    vec3 fragPosEye = vec3(view * vec4(FragPosWorld, 1.0));

    float dist = length(lampPos - FragPosWorld);
    float attenuation = 1.0 / (lampConst + lampLinear * dist + lampQuad * dist * dist);

    vec3 viewDir = normalize(-fragPosEye); // originea camerei (0,0,0) => directia din fragment spre camera =  (0,0,0) - fragPosEye
    vec3 Leye = normalize(lampPosEye - fragPosEye); // directia din fragment -> lumina
    vec3 normalEye = normalize(mat3(view) * N); 

    float diffCoeff = max(dot(normalEye, Leye), 0.0);
    vec3 reflectDir = reflect(-Leye, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    diff = diffCoeff * lampColor * attenuation;
    spec = specularStrength * specCoeff * lampColor * attenuation;
}

void main()
{
    // normalizam FLAT or SMOOTH
    vec3 N = normalize(uShadingMode == 1 ? NormalWorldFlat : NormalWorld);

    vec3 diffuseTex = texture(diffuseTexture, TexCoords).rgb; // culoarea materialului
    if (diffuseTex == vec3(0.0)) diffuseTex = vec3(1.0);

    vec3 specTex = texture(specularTexture, TexCoords).rgb;  // controleaza highlighul specular (Cat de lucios e materialul)
    if (specTex == vec3(0.0)) specTex = vec3(1.0);

    // Lanterna din mana
    vec3 amb, diff, spec;
    vec3 LflashW; // pt ca are nevoie ShadowCalc pt a calcula directia luminii
    computeFlashLight(N, amb, diff, spec, LflashW);

    // Shadow Mapping doar pe flashlight
    float shadow = ShadowCalc(FragPosLightSpace, N, LflashW);
    diff *= (1.0 - shadow);
    spec *= (1.0 - shadow);

    // Lampa statica
    vec3 lampDiff = vec3(0.0);
    vec3 lampSpec = vec3(0.0);
    if (lampOn == 1) {
        computeLampPointLight(N, lampDiff, lampSpec);
    }

    vec3 color = (amb + diff + lampDiff) * diffuseTex + (spec + lampSpec) * specTex;
    fColor = vec4(color, 1.0);
}
