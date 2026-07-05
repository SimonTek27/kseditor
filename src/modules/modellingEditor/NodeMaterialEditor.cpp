#include "NodeMaterialEditor.h"
#include <QUuid>

namespace ks {

int socketIdCounter = 0;
int nodeIdCounter = 0;

QString MaterialNode::generateNodeId() {
    return "Node_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QString MaterialNode::generateSocketId() {
    return "Socket_" + QString::number(++socketIdCounter);
}

MaterialNode::MaterialNode(const QString& n, MaterialNodeType t) : id(generateNodeId()), name(n), type(t), isSelected(false), isMuted(false) {}

MaterialNode::~MaterialNode() = default;

void MaterialNode::addInput(const QString& n, NodeInputType t, const QVariant& def) {
    NodeSocket socket;
    socket.id = generateSocketId();
    socket.name = n;
    socket.type = t;
    socket.defaultValue = def;
    socket.value = def;
    socket.isOutput = false;
    socket.isConnected = false;
    inputs.append(socket);
}

void MaterialNode::addOutput(const QString& n, NodeInputType t) {
    NodeSocket socket;
    socket.id = generateSocketId();
    socket.name = n;
    socket.type = t;
    socket.isOutput = true;
    socket.isConnected = false;
    outputs.append(socket);
}

NodeSocket* MaterialNode::findSocket(const QString& socketId) {
    for (auto& s : inputs) {
        if (s.id == socketId) return &s;
    }
    for (auto& s : outputs) {
        if (s.id == socketId) return &s;
    }
    return nullptr;
}

bool MaterialNode::processInputs() {
    for (auto& input : inputs) {
        if (input.isConnected && !input.connectedSocketId.isEmpty()) {
            for (auto* linkedNode : linkedNodes.values()) {
                for (auto& output : linkedNode->outputs) {
                    if (output.id == input.connectedSocketId) {
                        input.value = output.value;
                        break;
                    }
                }
            }
        }
    }
    return true;
}

QVariant MaterialNode::evaluateOutput(const QString& socketId) {
    for (auto& output : outputs) {
        if (output.id == socketId) {
            return output.value;
        }
    }
    return QVariant();
}

QString MaterialNode::getExpression() const {
    return name + "()";
}

QMap<QString, QString> MaterialNode::getGeneratedCode() const {
    QMap<QString, QString> code;
    QString expr = getExpression();
    if (expr != name + "()") {
        code["main"] = QString("// %1 (%2)\n").arg(name, id);
        code["decl"] = QString("vec4 %1_output = %2;\n").arg(id, expr);
    }
    return code;
}

OutputNode::OutputNode() : MaterialNode("Material Output", MaterialNodeType::Output) {
    addInput("Surface", NodeInputType::Shader);
    addInput("Displacement", NodeInputType::Float);
    addInput("Alpha", NodeInputType::Float);
}

QString OutputNode::getExpression() const {
    return "output_surface";
}

InputNode::InputNode(const QString& n, NodeInputType t) : MaterialNode(n, MaterialNodeType::Input) {
    addOutput("Value", t);
    setValue(QVariant());
}

void InputNode::setValue(const QVariant& val) {
    for (auto& out : outputs) {
        out.value = val;
    }
    if (!inputs.isEmpty()) {
        inputs[0].value = val;
    }
}

QVariant InputNode::getValue() const {
    if (!outputs.isEmpty()) {
        return outputs[0].value;
    }
    return QVariant();
}

QString InputNode::getExpression() const {
    return name.toLower().replace(" ", "_");
}

TextureNode::TextureNode() : MaterialNode("Texture", MaterialNodeType::Texture) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("Color", NodeInputType::Float4);
    addOutput("Alpha", NodeInputType::Float);
    texturePath = "";
    colorSpace = "sRGB";
}

QString TextureNode::getExpression() const {
    if (!texturePath.isEmpty()) {
        return "texture(\"" + texturePath + "\", uv)";
    }
    return "vec4(1.0)";
}

ImageNode::ImageNode() : MaterialNode("Image Texture", MaterialNodeType::Texture) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("Color", NodeInputType::Float4);
    addOutput("Alpha", NodeInputType::Float);
}

QString ImageNode::getExpression() const {
    if (!texturePath.isEmpty()) {
        return "texture(\"" + texturePath + "\", uv)";
    }
    return "vec4(1.0)";
}

CubeMapNode::CubeMapNode() : MaterialNode("Environment Map", MaterialNodeType::Texture) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("Color", NodeInputType::Float4);
}

QString CubeMapNode::getExpression() const {
    if (!texturePath.isEmpty()) {
        return "textureCube(\"" + texturePath + "\", dir)";
    }
    return "vec4(0.5, 0.5, 0.5, 1.0)";
}

NormalMapNode::NormalMapNode() : MaterialNode("Normal Map", MaterialNodeType::Texture) {
    addInput("Strength", NodeInputType::Float);
    addInput("Color", NodeInputType::Color);
    addOutput("Normal", NodeInputType::Float3);
}

QString NormalMapNode::getExpression() const {
    if (!texturePath.isEmpty()) {
        return "normalize(texture(\"" + texturePath + "\", uv).xyz * 2.0 - 1.0)";
    }
    return "vec3(0.0, 0.0, 1.0)";
}

ColorRampNode::ColorRampNode() : MaterialNode("Color Ramp", MaterialNodeType::Color) {
    addInput("Fac", NodeInputType::Float);
    addOutput("Color", NodeInputType::Color);
    interpolation = 1;
    stops.append({0.0f, Qt::black});
    stops.append({1.0f, Qt::white});
}

QString ColorRampNode::getExpression() const {
    return "color_ramp(fac)";
}

MixNode::MixNode() : MaterialNode("Mix", MaterialNodeType::Shader) {
    addInput("A", NodeInputType::Float);
    addInput("B", NodeInputType::Float);
    addInput("Factor", NodeInputType::Float);
    addOutput("Result", NodeInputType::Float);
    mixType = MixType::Mix;
}

QString MixNode::getExpression() const {
    switch (mixType) {
        case MixType::Mix: return "mix(a, b, factor)";
        case MixType::Add: return "a + b";
        case MixType::Subtract: return "a - b";
        case MixType::Multiply: return "a * b";
        case MixType::Divide: return "a / b";
        case MixType::Power: return "pow(a, b)";
        default: return "mix(a, b, factor)";
    }
}

MathNode::MathNode() : MaterialNode("Math", MaterialNodeType::Math) {
    addInput("Value", NodeInputType::Float);
    addInput("Value", NodeInputType::Float);
    addOutput("Result", NodeInputType::Float);
    mathType = MathType::Add;
}

QString MathNode::getExpression() const {
    switch (mathType) {
        case MathType::Add: return "a + b";
        case MathType::Subtract: return "a - b";
        case MathType::Multiply: return "a * b";
        case MathType::Divide: return "a / b";
        case MathType::Power: return "pow(a, b)";
        case MathType::Abs: return "abs(a)";
        case MathType::Sin: return "sin(a)";
        case MathType::Cos: return "cos(a)";
        case MathType::Tan: return "tan(a)";
        default: return "a";
    }
}

VectorMathNode::VectorMathNode() : MaterialNode("Vector Math", MaterialNodeType::Vector) {
    addInput("Vector", NodeInputType::Float3);
    addInput("Vector", NodeInputType::Float3);
    addOutput("Vector", NodeInputType::Float3);
    addOutput("Value", NodeInputType::Float);
    vectorType = VectorType::Add;
}

QString VectorMathNode::getExpression() const {
    switch (vectorType) {
        case VectorType::Add: return "a + b";
        case VectorType::Subtract: return "a - b";
        case VectorType::Multiply: return "a * b";
        case VectorType::Normalize: return "normalize(a)";
        default: return "a";
    }
}

SeparateXYZNode::SeparateXYZNode() : MaterialNode("Separate XYZ", MaterialNodeType::Utility) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("X", NodeInputType::Float);
    addOutput("Y", NodeInputType::Float);
    addOutput("Z", NodeInputType::Float);
}

QString SeparateXYZNode::getExpression() const {
    if (inputs.size() >= 1 && !inputs[0].connectedSocketId.isEmpty()) {
        QString srcId = inputs[0].connectedSocketId;
        srcId.replace('-', '_');
        return "vec4(" + srcId + ".x, 0, 0, 0)";
    }
    return "vec4(0, 0, 0, 0)";
}

CombineXYZNode::CombineXYZNode() : MaterialNode("Combine XYZ", MaterialNodeType::Utility) {
    addInput("X", NodeInputType::Float);
    addInput("Y", NodeInputType::Float);
    addInput("Z", NodeInputType::Float);
    addOutput("Vector", NodeInputType::Float3);
}

QString CombineXYZNode::getExpression() const {
    return "vec3(x, y, z)";
}

RGBToBWNode::RGBToBWNode() : MaterialNode("RGB to BW", MaterialNodeType::Color) {
    addInput("Color", NodeInputType::Color);
    addOutput("Val", NodeInputType::Float);
}

QString RGBToBWNode::getExpression() const {
    return "dot(color.rgb, vec3(0.299, 0.587, 0.114))";
}

FresnelNode::FresnelNode() : MaterialNode("Fresnel", MaterialNodeType::Shader) {
    addInput("IOR", NodeInputType::Float);
    addInput("Normal", NodeInputType::Float3);
    addOutput("Fac", NodeInputType::Float);
    ior = 1.45f;
}

QString FresnelNode::getExpression() const {
    return "pow(1.0 - dot(normal, view_dir), ior)";
}

AmbientOcclusionNode::AmbientOcclusionNode() : MaterialNode("Ambient Occlusion", MaterialNodeType::Shader) {
    addInput("Normal", NodeInputType::Float3);
    addInput("Distance", NodeInputType::Float);
    addOutput("AO", NodeInputType::Float);
    samples = 16;
    distance = 1.0f;
    strength = 1.0f;
}

QString AmbientOcclusionNode::getExpression() const {
    return "ao";
}

BevelNode::BevelNode() : MaterialNode("Bevel", MaterialNodeType::Shader) {
    addInput("Normal", NodeInputType::Float3);
    addOutput("Normal", NodeInputType::Float3);
    radius = 0.05f;
}

QString BevelNode::getExpression() const {
    return "bevel";
}

EmissionNode::EmissionNode() : MaterialNode("Emission", MaterialNodeType::Shader) {
    addInput("Color", NodeInputType::Color);
    addInput("Strength", NodeInputType::Float);
    addOutput("Emission", NodeInputType::Float3);
    strength = 1.0f;
}

QString EmissionNode::getExpression() const {
    return "color * strength";
}

BSDFPrincipledNode::BSDFPrincipledNode() : MaterialNode("Principled BSDF", MaterialNodeType::Shader) {
    addInput("Base Color", NodeInputType::Color);
    addInput("Metallic", NodeInputType::Float);
    addInput("Roughness", NodeInputType::Float);
    addInput("Specular", NodeInputType::Float);
    addInput("IOR", NodeInputType::Float);
    addInput("Normal", NodeInputType::Float3);
    addInput("Emission", NodeInputType::Color);
    addInput("Alpha", NodeInputType::Float);
    addOutput("BSDF", NodeInputType::Shader);

    subsurface = 0.0f;
    metallic = 0.0f;
    specular = 0.5f;
    roughness = 0.5f;
    clearcoat = 0.0f;
    clearcoatRoughness = 0.0f;
    ior = 1.45f;
    transmission = 0.0f;
    thickness = 0.0f;
    emissionStrength = 1.0f;
}

QString BSDFPrincipledNode::getExpression() const {
    return "principled_bsdf";
}

ShaderToRGBNode::ShaderToRGBNode() : MaterialNode("Shader To RGB", MaterialNodeType::Utility) {
    addInput("Shader", NodeInputType::Shader);
    addOutput("Color", NodeInputType::Color);
}

QString ShaderToRGBNode::getExpression() const {
    return "shader_to_rgb";
}

RGBToShaderNode::RGBToShaderNode() : MaterialNode("RGB To Shader", MaterialNodeType::Utility) {
    addInput("Color", NodeInputType::Color);
    addOutput("Shader", NodeInputType::Shader);
}

QString RGBToShaderNode::getExpression() const {
    return "rgb_to_shader";
}

NoiseTextureNode::NoiseTextureNode() : MaterialNode("Noise Texture", MaterialNodeType::Texture) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("Value", NodeInputType::Float);
    addOutput("Color", NodeInputType::Float4);
    scale = 1.0f;
    detail = 16.0f;
    distortion = 0.0f;
}

QString NoiseTextureNode::getExpression() const {
    return "noise(uv * scale)";
}

VoronoiNode::VoronoiNode() : MaterialNode("Voronoi Texture", MaterialNodeType::Texture) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("Distance", NodeInputType::Float);
    addOutput("Color", NodeInputType::Float4);
    scale = 1.0f;
    detail = 2;
    metric = Metric::Euclidean;
    feature = Feature::F1;
}

QString VoronoiNode::getExpression() const {
    return "voronoi(uv * scale)";
}

WaveTextureNode::WaveTextureNode() : MaterialNode("Wave Texture", MaterialNodeType::Texture) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("Fac", NodeInputType::Float);
    waveType = WaveType::Sine;
    direction = WaveDirection::X;
    scale = 1.0f;
    distortion = 0.0f;
    detail = 16.0f;
}

QString WaveTextureNode::getExpression() const {
    return "wave(uv * scale)";
}

GradientTextureNode::GradientTextureNode() : MaterialNode("Gradient Texture", MaterialNodeType::Texture) {
    addInput("Fac", NodeInputType::Float);
    addOutput("Color", NodeInputType::Color);
    gradientType = GradientType::Linear;
}

QString GradientTextureNode::getExpression() const {
    return "gradient(fac)";
}

MappingNode::MappingNode() : MaterialNode("Mapping", MaterialNodeType::Utility) {
    addInput("Vector", NodeInputType::Float3);
    addOutput("Vector", NodeInputType::Float3);
    location = QVector3D(0, 0, 0);
    rotation = QVector3D(0, 0, 0);
    scale = QVector3D(1, 1, 1);
}

QString MappingNode::getExpression() const {
    return "map";
}

TextureCoordinateNode::TextureCoordinateNode() : MaterialNode("Texture Coordinate", MaterialNodeType::Input) {
    addOutput("Generated", NodeInputType::Float3);
    addOutput("Normal", NodeInputType::Float3);
    addOutput("UV", NodeInputType::Float3);
    addOutput("Object", NodeInputType::Float3);
    addOutput("Camera", NodeInputType::Float3);
    uvMap = UVMap::UV;
}

QString TextureCoordinateNode::getExpression() const {
    return "uv";
}

BrightContrastNode::BrightContrastNode() : MaterialNode("Brightness/Contrast", MaterialNodeType::Color) {
    addInput("Color", NodeInputType::Color);
    addOutput("Color", NodeInputType::Color);
    brightness = 0.0f;
    contrast = 0.0f;
}

QString BrightContrastNode::getExpression() const {
    return "color * (1.0 + contrast) + brightness";
}

HSVNode::HSVNode() : MaterialNode("Hue Saturation Value", MaterialNodeType::Color) {
    addInput("Color", NodeInputType::Color);
    addInput("Hue", NodeInputType::Float);
    addInput("Saturation", NodeInputType::Float);
    addInput("Value", NodeInputType::Float);
    addOutput("Color", NodeInputType::Color);
    mode = Mode::Combine;
}

QString HSVNode::getExpression() const {
    return "rgb_to_hsv(color)";
}

MaterialGraph::MaterialGraph() : outputNode(nullptr) {}

MaterialGraph::~MaterialGraph() {
    clear();
}

MaterialNode* MaterialGraph::createNode(const QString& type, const QPointF& position) {
    MaterialNode* node = nullptr;

    if (type == "Output") {
        node = new OutputNode();
    } else if (type == "ImageTexture" || type == "Texture") {
        node = new ImageNode();
    } else if (type == "PrincipledBSDF" || type == "BSDF") {
        node = new BSDFPrincipledNode();
    } else if (type == "Mix") {
        node = new MixNode();
    } else if (type == "Math") {
        node = new MathNode();
    } else if (type == "SeparateXYZ") {
        node = new SeparateXYZNode();
    } else if (type == "CombineXYZ") {
        node = new CombineXYZNode();
    } else if (type == "Mapping") {
        node = new MappingNode();
    } else if (type == "NoiseTexture") {
        node = new NoiseTextureNode();
    } else if (type == "ColorRamp") {
        node = new ColorRampNode();
    } else if (type == "VectorMath") {
        node = new VectorMathNode();
    } else if (type == "RGBToBW") {
        node = new RGBToBWNode();
    } else if (type == "Fresnel") {
        node = new FresnelNode();
    } else if (type == "Emission") {
        node = new EmissionNode();
    } else if (type == "TextureCoordinate") {
        node = new TextureCoordinateNode();
    } else if (type == "BrightContrast") {
        node = new BrightContrastNode();
    } else if (type == "HSV") {
        node = new HSVNode();
    } else if (type == "GradientTexture") {
        node = new GradientTextureNode();
    } else if (type == "WaveTexture") {
        node = new WaveTextureNode();
    } else if (type == "Voronoi") {
        node = new VoronoiNode();
    } else if (type == "ShaderToRGB") {
        node = new ShaderToRGBNode();
    } else if (type == "RGBToShader") {
        node = new RGBToShaderNode();
    } else if (type == "AmbientOcclusion") {
        node = new AmbientOcclusionNode();
    } else if (type == "Bevel") {
        node = new BevelNode();
    } else if (type == "NormalMap") {
        node = new NormalMapNode();
    } else if (type == "CubeMap") {
        node = new CubeMapNode();
    }

    if (node) {
        node->position = position;
        nodes.append(node);

        if (type == "Output") {
            outputNode = node;
        }
    }

    return node;
}

void MaterialGraph::deleteNode(const QString& nodeId) {
    for (int i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->id == nodeId) {
            delete nodes[i];
            nodes.removeAt(i);
            if (outputNode && outputNode->id == nodeId) {
                outputNode = nullptr;
            }
            break;
        }
    }
    updateLinks(nodeId);
}

void MaterialGraph::connectNodes(const QString& fromNodeId, const QString& fromSocket,
                                  const QString& toNodeId, const QString& toSocket) {
    MaterialNode* fromNode = findNode(fromNodeId);
    MaterialNode* toNode = findNode(toNodeId);

    if (fromNode && toNode) {
        fromNode->linkedNodes[fromSocket] = toNode;

        for (auto& s : fromNode->outputs) {
            if (s.id == fromSocket) {
                s.isConnected = true;
                break;
            }
        }

        for (auto& s : toNode->inputs) {
            if (s.id == toSocket) {
                s.isConnected = true;
                s.connectedSocketId = fromSocket;
                break;
            }
        }
    }
}

void MaterialGraph::disconnectNodes(const QString& fromNodeId, const QString& fromSocket,
                                     const QString& toNodeId, const QString& toSocket) {
    MaterialNode* fromNode = findNode(fromNodeId);
    MaterialNode* toNode = findNode(toNodeId);

    if (fromNode) {
        fromNode->linkedNodes.remove(fromSocket);
        for (auto& s : fromNode->outputs) {
            if (s.id == fromSocket) {
                s.isConnected = false;
                break;
            }
        }
    }

    if (toNode) {
        for (auto& s : toNode->inputs) {
            if (s.id == toSocket) {
                s.isConnected = false;
                s.connectedSocketId = "";
                break;
            }
        }
    }
}

MaterialNode* MaterialGraph::findNode(const QString& nodeId) {
    for (auto* node : nodes) {
        if (node->id == nodeId) return node;
    }
    return nullptr;
}

QVector<MaterialNode*> MaterialGraph::getInputNodes() {
    QVector<MaterialNode*> inputs;
    for (auto* node : nodes) {
        if (node->type == MaterialNodeType::Input) {
            inputs.append(node);
        }
    }
    return inputs;
}

QVector<MaterialNode*> MaterialGraph::topologicalSort() {
    QVector<MaterialNode*> sorted;
    QSet<QString> visited;
    QSet<QString> inStack;

    std::function<bool(MaterialNode*)> visit = [&](MaterialNode* node) -> bool {
        if (inStack.contains(node->id)) return true;
        if (visited.contains(node->id)) return false;

        visited.insert(node->id);
        inStack.insert(node->id);

        for (auto* linked : node->linkedNodes.values()) {
            if (visit(linked)) return true;
        }

        inStack.remove(node->id);
        sorted.prepend(node);
        return false;
    };

    for (auto* node : nodes) {
        if (!visited.contains(node->id)) {
            visit(node);
        }
    }

    return sorted;
}

QString MaterialGraph::generateGLSL() {
    QString code = "#version 330 core\n\nin vec2 vTexCoord;\nout vec4 fragColor;\n\n";

    // Uniforms from input/texture nodes
    for (auto* node : nodes) {
        if (node->type == MaterialNodeType::Input) {
            for (const auto& s : node->outputs) {
                QString varName = node->id;
                varName.replace('-', '_');
                if (s.type == NodeInputType::Image)
                    code += "uniform sampler2D " + varName + ";\n";
                else if (s.type == NodeInputType::Color)
                    code += "const vec4 " + varName + " = vec4(" + s.value.toString() + ");\n";
                else if (s.type == NodeInputType::Float)
                    code += "const float " + varName + " = " + s.value.toString() + ";\n";
            }
        }
        if (node->type == MaterialNodeType::Texture) {
            QString varName = node->id;
            varName.replace('-', '_');
            code += "uniform sampler2D " + varName + ";\n";
        }
    }

    code += "\nvoid main() {\n";

    // Declare intermediate variables from topological sort
    QVector<MaterialNode*> sorted = topologicalSort();
    for (auto* node : sorted) {
        if (node->type == MaterialNodeType::Input || node->type == MaterialNodeType::Texture)
            continue; // Already declared in uniform section
        QString varName = node->id;
        varName.replace('-', '_');
        QString expr = node->getExpression();
        // Replace socket references with variable names
        for (const auto& s : node->inputs) {
            if (s.isConnected && !s.connectedSocketId.isEmpty() && node->linkedNodes.contains(s.connectedSocketId)) {
                MaterialNode* src = node->linkedNodes[s.connectedSocketId];
                QString srcVar = src->id;
                srcVar.replace('-', '_');
                QString socketRef = s.id;
                socketRef.replace('-', '_');
                expr.replace(socketRef, srcVar);
            }
        }
        // Wrap non-vec4 expressions so the vec4 declaration is valid GLSL
        NodeInputType outType = node->outputs.isEmpty() ? NodeInputType::Float4 : node->outputs[0].type;
        if (outType == NodeInputType::Float)
            expr = "vec4(" + expr + ", 0.0, 0.0, 1.0)";
        else if (outType == NodeInputType::Float2)
            expr = "vec4(" + expr + ", 0.0, 1.0)";
        else if (outType == NodeInputType::Float3 || outType == NodeInputType::Color)
            expr = "vec4(" + expr + ", 1.0)";
        code += "    vec4 " + varName + " = " + expr + ";\n";
    }

    // Find output node and use its surface input as final color
    QString outVar = "vec4(1.0)";
    for (auto* node : sorted) {
        if (dynamic_cast<OutputNode*>(node)) {
            for (const auto& s : node->inputs) {
                if (s.isConnected && !s.connectedSocketId.isEmpty() && node->linkedNodes.contains(s.connectedSocketId)) {
                    MaterialNode* src = node->linkedNodes[s.connectedSocketId];
                    outVar = src->id;
                    outVar.replace('-', '_');
                }
            }
        }
    }
    code += "    fragColor = " + outVar + ";\n";
    code += "}\n";
    return code;
}

QString MaterialGraph::generateHLSL() {
    QString code = "cbuffer MaterialConstants {\n";
    for (auto* node : nodes) {
        for (const auto& s : node->inputs) {
            if (s.type == NodeInputType::Float) {
                QString hName = s.id;
                hName.replace('-', '_');
                code += "    float " + hName + " : packoffset(c0);\n";
            }
        }
    }
    code += "};\n\n";

    for (auto* node : nodes) {
        if (node->type == MaterialNodeType::Texture) {
            QString varName = node->id;
            varName.replace('-', '_');
            code += "Texture2D " + varName + " : register(t0);\n";
            code += "SamplerState " + varName + "_sampler : register(s0);\n\n";
        }
    }

    QVector<MaterialNode*> sorted = topologicalSort();
    for (auto* node : sorted) {
        if (node->type == MaterialNodeType::Input || node->type == MaterialNodeType::Texture)
            continue;
        QString varName = node->id;
        varName.replace('-', '_');
        QString expr = node->getExpression();
        for (const auto& s : node->inputs) {
            if (s.isConnected && !s.connectedSocketId.isEmpty() && node->linkedNodes.contains(s.connectedSocketId)) {
                MaterialNode* src = node->linkedNodes[s.connectedSocketId];
                QString srcVar = src->id;
                srcVar.replace('-', '_');
                QString socketRef = s.id;
                socketRef.replace('-', '_');
                expr.replace(socketRef, srcVar);
            }
        }
        NodeInputType outType = node->outputs.isEmpty() ? NodeInputType::Float4 : node->outputs[0].type;
        if (outType == NodeInputType::Float)
            expr = "float4(" + expr + ", 0.0, 0.0, 1.0)";
        else if (outType == NodeInputType::Float2)
            expr = "float4(" + expr + ", 0.0, 1.0)";
        else if (outType == NodeInputType::Float3 || outType == NodeInputType::Color)
            expr = "float4(" + expr + ", 1.0)";
        code += "float4 " + varName + " = " + expr + ";\n";
    }

    QString outVar = "float4(1.0, 1.0, 1.0, 1.0)";
    for (auto* node : sorted) {
        if (dynamic_cast<OutputNode*>(node)) {
            for (const auto& s : node->inputs) {
                if (s.isConnected && !s.connectedSocketId.isEmpty() && node->linkedNodes.contains(s.connectedSocketId)) {
                    MaterialNode* src = node->linkedNodes[s.connectedSocketId];
                    outVar = src->id;
                    outVar.replace('-', '_');
                }
            }
        }
    }

    code += "\nfloat4 mainPS() : SV_TARGET {\n    return " + outVar + ";\n}\n";
    return code;
}

void MaterialGraph::clear() {
    for (auto* node : nodes) {
        delete node;
    }
    nodes.clear();
    outputNode = nullptr;
}

void MaterialGraph::copyFrom(const MaterialGraph& other) {
    clear();
    name = other.name;

    // Map old node IDs to new nodes
    QMap<QString, MaterialNode*> nodeMap;

    for (auto* node : other.nodes) {
        MaterialNode* newNode = createNode(node->name, node->position);
        if (newNode) {
            newNode->id = node->id;
            newNode->size = node->size;
            newNode->isSelected = node->isSelected;
            newNode->isMuted = node->isMuted;
            // Copy inputs
            newNode->inputs = node->inputs;
            // Copy outputs
            newNode->outputs = node->outputs;
            nodeMap[node->id] = newNode;
        }
    }

    // Rebuild linkedNodes map using new node pointers
    for (auto* node : other.nodes) {
        MaterialNode* newNode = nodeMap.value(node->id);
        if (newNode) {
            for (auto it = node->linkedNodes.constBegin(); it != node->linkedNodes.constEnd(); ++it) {
                MaterialNode* linkedNew = nodeMap.value(it.value()->id);
                if (linkedNew)
                    newNode->linkedNodes[it.key()] = linkedNew;
            }
        }
    }
}

void MaterialGraph::updateLinks(const QString& removedNodeId) {
    // Remove linkedNodes entries pointing to the deleted node
    for (auto* node : nodes) {
        QStringList toRemove;
        for (auto it = node->linkedNodes.constBegin(); it != node->linkedNodes.constEnd(); ++it) {
            if (it.value()->id == removedNodeId)
                toRemove.append(it.key());
        }
        for (const QString& key : toRemove) {
            node->linkedNodes.remove(key);
        }
        // Clear input sockets whose linked node was removed
        for (auto& s : node->inputs) {
            if (s.isConnected && !s.connectedSocketId.isEmpty()) {
                bool linkStillValid = false;
                for (auto* ln : node->linkedNodes.values()) {
                    for (const auto& os : ln->outputs) {
                        if (os.id == s.connectedSocketId) {
                            linkStillValid = true;
                            break;
                        }
                    }
                    if (linkStillValid) break;
                }
                if (!linkStillValid) {
                    s.isConnected = false;
                    s.connectedSocketId = "";
                }
            }
        }
    }
}

MaterialGraph* NodeGroup::toGraph() const {
    auto* graph = new MaterialGraph();
    for (auto* node : nodes) {
        graph->nodes.append(node);
    }
    return graph;
}

NodeGroup* NodeGroup::fromGraph(const MaterialGraph& graph) {
    auto* group = new NodeGroup();
    for (auto* node : graph.nodes) {
        group->nodes.append(node);
    }
    return group;
}

MaterialNodeEditor::MaterialNodeEditor() : currentGraph(nullptr) {}

MaterialNodeEditor::~MaterialNodeEditor() {
    for (auto* graph : graphs) {
        delete graph;
    }
    for (auto* group : groups) {
        delete group;
    }
}

MaterialGraph* MaterialNodeEditor::newGraph() {
    auto* graph = new MaterialGraph();
    graph->name = "Material " + QString::number(graphs.size() + 1);
    graphs.append(graph);
    setCurrentGraph(graph);
    return graph;
}

void MaterialNodeEditor::setCurrentGraph(MaterialGraph* graph) {
    currentGraph = graph;
}

MaterialGraph* MaterialNodeEditor::getCurrentGraph() {
    return currentGraph;
}

void MaterialNodeEditor::registerDefaultNodes() {
    nodeFactory["Output"] = []{ return new OutputNode(); };
    nodeFactory["ImageTexture"] = []{ return new ImageNode(); };
    nodeFactory["PrincipledBSDF"] = []{ return new BSDFPrincipledNode(); };
    nodeFactory["Mix"] = []{ return new MixNode(); };
    nodeFactory["Math"] = []{ return new MathNode(); };
    nodeFactory["VectorMath"] = []{ return new VectorMathNode(); };
    nodeFactory["SeparateXYZ"] = []{ return new SeparateXYZNode(); };
    nodeFactory["CombineXYZ"] = []{ return new CombineXYZNode(); };
    nodeFactory["Mapping"] = []{ return new MappingNode(); };
    nodeFactory["NoiseTexture"] = []{ return new NoiseTextureNode(); };
    nodeFactory["ColorRamp"] = []{ return new ColorRampNode(); };
    nodeFactory["RGBToBW"] = []{ return new RGBToBWNode(); };
    nodeFactory["Fresnel"] = []{ return new FresnelNode(); };
    nodeFactory["Emission"] = []{ return new EmissionNode(); };
    nodeFactory["TextureCoordinate"] = []{ return new TextureCoordinateNode(); };
    nodeFactory["BrightContrast"] = []{ return new BrightContrastNode(); };
    nodeFactory["HSV"] = []{ return new HSVNode(); };
    nodeFactory["GradientTexture"] = []{ return new GradientTextureNode(); };
    nodeFactory["WaveTexture"] = []{ return new WaveTextureNode(); };
    nodeFactory["Voronoi"] = []{ return new VoronoiNode(); };
    nodeFactory["ShaderToRGB"] = []{ return new ShaderToRGBNode(); };
    nodeFactory["RGBToShader"] = []{ return new RGBToShaderNode(); };
    nodeFactory["AmbientOcclusion"] = []{ return new AmbientOcclusionNode(); };
    nodeFactory["Bevel"] = []{ return new BevelNode(); };
    nodeFactory["NormalMap"] = []{ return new NormalMapNode(); };
    nodeFactory["CubeMap"] = []{ return new CubeMapNode(); };
}

QStringList MaterialNodeEditor::getAvailableNodeTypes() {
    return nodeFactory.keys();
}

}