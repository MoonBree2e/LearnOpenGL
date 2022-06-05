#version 330 core
out vec4 FragColor;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct Light {
    // vec3 position; // 使用定向光就不再需要了
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform float time;

void main(){
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.f);
    vec3 diffuse = diff * light.diffuse * texture(material.diffuse, TexCoords).rgb;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

    vec3 resultColor = ambient + diffuse + specular;
    FragColor = vec4(resultColor, 1.f);
}