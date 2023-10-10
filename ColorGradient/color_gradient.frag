#version 460 core
out vec4 FragColor;

struct ControlPoint {
    vec4 position;
    vec4 color;
};

layout(std140, binding = 0) buffer ControlPointsSSBO {
    ControlPoint controlPoints[];
};

layout(std140, binding = 1) uniform ControlPointsUBO {
    int numControlPoints;
};
    
void main()
{
    float p = gl_FragCoord.x / 1280.0;
    vec4 color;
    int index = 0;

    // 控制点为0
    if(numControlPoints == 0) {
        FragColor = vec4(1);
        return;
    }

    for (int i = 0; i < numControlPoints; i++) {
        if (p > controlPoints[i].position.x && i == numControlPoints - 1) {
            index = i;
            break;
        }
        if (p > controlPoints[i].position.x && p <= controlPoints[i+1].position.x) {
            index = i;
            break;
        }
    }

    // 最后一个控制点之后
    if(index==numControlPoints-1) {
        FragColor = controlPoints[index].color;
        return;
    }


    float t = (p - controlPoints[index].position.x) / (controlPoints[index + 1].position.x - controlPoints[index].position.x);
    color = controlPoints[index + 1].color * t + controlPoints[index].color * (1-t);

    FragColor = color;
}