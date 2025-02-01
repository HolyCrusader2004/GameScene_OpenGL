#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

uniform vec3 lightDir;  
uniform vec3 lightColor;  

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

uniform bool showThunder;

struct PointLight {
    vec3 position; 
    float constant;
    float linear;
    float quadratic;
    vec3 color;
};

struct Flashlight {
    vec3 position;     
    vec3 direction;    
    float cutOff;      
    float outerCutOff; 
    vec3 color;
    bool activated;
};

struct SpotLight {
    vec3 position;     
    vec3 direction;    
    float cutOff;      
    float outerCutOff; 
    vec3 color;
};

uniform PointLight pointLights[3];
uniform Flashlight flashlight;
uniform SpotLight spotLights[2];

uniform mat4 view; 

// Lighting constants
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;
vec3 viewDirN;

void computeLightComponents() {       
    vec3 cameraPosEye = vec3(0.0f); 

    vec3 normalEye = normalize(fNormal);    

    vec3 lightDirN = normalize(lightDir);
    
    viewDirN = normalize(cameraPosEye - fPosEye.xyz);
        
    ambient = ambientStrength * lightColor;
    
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    
    vec3 reflection = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;
}

void computePointLightComponents(vec3 lightPosWorld, vec3 lightColor, float constant, float linear, float quadratic) {

    vec3 lightPosEye = (view * vec4(lightPosWorld, 1.0f)).xyz;

    vec3 normalEye = normalize(fNormal); 
    vec3 lightDirN = normalize(lightPosEye - fPosEye.xyz);
    vec3 reflection = reflect(-lightDirN, normalEye);  
        
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    float dist = length(lightPosEye - fPosEye.xyz);
    float attenuation = 1.0 / (constant + linear * dist + quadratic * (dist * dist));

    ambient += attenuation * ambientStrength * lightColor;  
    diffuse += attenuation * max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    specular += attenuation * specularStrength * specCoeff * lightColor;
}

void computeFlashlightEffect() {
    
    if(!flashlight.activated){
        return;
    }
    vec3 flashlightPosEye = (view * vec4(flashlight.position, 1.0f)).xyz;
    vec3 flashlightDirEye = normalize(mat3(view) * flashlight.direction); 

    vec3 normalEye = normalize(fNormal); 
    vec3 lightDirNflashlight = normalize(flashlightPosEye - fPosEye.xyz);
    float theta = dot(lightDirNflashlight, -flashlightDirEye);  
        
    float epsilon = flashlight.cutOff - flashlight.outerCutOff;
    float intensity = clamp((theta - flashlight.outerCutOff) / epsilon, 0.0f, 1.0f);

    vec3 reflection = reflect(-lightDirNflashlight, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
        
    ambient += flashlight.color * ambientStrength * intensity;
    diffuse += intensity * max(dot(normalEye, lightDirNflashlight), 0.0f) * flashlight.color * intensity;
    specular += specCoeff * flashlight.color * specularStrength * intensity;

}

void computeSpotLight(vec3 lightPosWorld, vec3 lightDirection, vec3 lightColor, float cutoff, float outerCutoff) {
    vec3 lightPosEye = (view * vec4(lightPosWorld, 1.0f)).xyz;
    vec3 lightDirEye = normalize(mat3(view) * lightDirection);

    vec3 normalEye = normalize(fNormal);
    vec3 lightDirToFrag = normalize(lightPosEye - fPosEye.xyz);

    float theta = dot(lightDirToFrag, -lightDirEye);

    float epsilon = cutoff - outerCutoff;
    float intensity = clamp((theta - outerCutoff) / epsilon, 0.0f, 1.0f);

    ambient += lightColor * ambientStrength * intensity;

    float diff = max(dot(normalEye, lightDirToFrag), 0.0f);
    diffuse += intensity * diff * lightColor * intensity;

    vec3 reflection = reflect(-lightDirToFrag, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    specular += intensity * specCoeff * lightColor * specularStrength;
}


float computeShadow() {
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    normalizedCoords = normalizedCoords * 0.5 + 0.5; 

    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    float currentDepth = normalizedCoords.z;

    if (normalizedCoords.z > 1.0f)
        return 0.0f;

    float shadow = currentDepth > closestDepth + 0.005 ? 1.0 : 0.0;
    return shadow;
}

float computeFog() {
    float fogDensity = 0.005f;
    float fragmentDistance = length(fPosEye); 
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    return clamp(fogFactor, 0.0f, 1.0f);
}

void main() {
    vec4 colorFromTexture = texture(diffuseTexture, fTexCoords);

    if (colorFromTexture.a < 0.1)
        discard;

    computeLightComponents();
    computeFlashlightEffect();
    for (int i = 0; i < 3; i++) {
        computePointLightComponents(
            pointLights[i].position,  
            pointLights[i].color,       
            pointLights[i].constant,   
            pointLights[i].linear,     
            pointLights[i].quadratic
        );
    }

    for (int i = 0; i < 2; i++) {
        computeSpotLight(
            spotLights[i].position,  
            spotLights[i].direction,       
            spotLights[i].color,   
            spotLights[i].cutOff,     
            spotLights[i].outerCutOff
        );
    }

    ambient *= texture(diffuseTexture, fTexCoords).rgb;
    diffuse *= texture(diffuseTexture, fTexCoords).rgb;
    specular *= texture(specularTexture, fTexCoords).rgb;

    float shadow = computeShadow();

    vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);
    color = color * 0.05;
    
    if (showThunder) {
        color = color * 100;    
    }
    float fogFactor = computeFog();

    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

    fColor = mix(fogColor, vec4(color, 0.8), fogFactor);
}
