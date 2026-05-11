#include "D3D11Renderer.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

namespace bs3d {

namespace {

std::string formatHresult(HRESULT hr) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<unsigned long>(hr);
    return out.str();
}

void assignError(std::string* error, const std::string& message) {
    if (error != nullptr) {
        *error = message;
    }
}

template <typename T>
void releaseAndNull(T*& ptr) {
    if (ptr != nullptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

struct Float3 {
    float x, y, z;
};

struct Matrix4 {
    float values[16];
};

struct Vertex {
    float position[3];
};

struct LineVertex {
    float position[3];
    float color[4];
};

struct alignas(16) PrimitiveConstants {
    float mvp[16];
    float color[4];
};

constexpr UINT DebugLineVertexCapacity = 4096;

static_assert(sizeof(PrimitiveConstants) % 16 == 0, "D3D11 constant buffers must be 16-byte aligned");

float dot3(const Float3& a, const Float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length3(const Float3& v) {
    return std::sqrt(dot3(v, v));
}

Float3 normalize3(const Float3& v) {
    const float len = length3(v);
    if (len <= 1e-7f) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {v.x / len, v.y / len, v.z / len};
}

Float3 cross3(const Float3& a, const Float3& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

Float3 subtract3(const Float3& a, const Float3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

// Row-major look-at view matrix (row vectors, mul(row_vector, matrix) convention).
// This is the transpose of the classic column-major lookAt.
Matrix4 lookAtMatrix(const Float3& eye, const Float3& target, const Float3& up) {
    const Float3 forward = normalize3(subtract3(target, eye));
    const Float3 right = normalize3(cross3(up, forward));
    const Float3 correctedUp = cross3(forward, right);

    // Row-major: row 0 = right, row 1 = up, row 2 = forward, row 3 = translation.
    Matrix4 matrix{};
    matrix.values[0]  = right.x;
    matrix.values[1]  = correctedUp.x;
    matrix.values[2]  = forward.x;
    matrix.values[3]  = 0.0f;

    matrix.values[4]  = right.y;
    matrix.values[5]  = correctedUp.y;
    matrix.values[6]  = forward.y;
    matrix.values[7]  = 0.0f;

    matrix.values[8]  = right.z;
    matrix.values[9]  = correctedUp.z;
    matrix.values[10] = forward.z;
    matrix.values[11] = 0.0f;

    matrix.values[12] = -dot3(right, eye);
    matrix.values[13] = -dot3(correctedUp, eye);
    matrix.values[14] = -dot3(forward, eye);
    matrix.values[15] = 1.0f;

    return matrix;
}


Matrix4 identityMatrix() {
    return Matrix4{{1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f}};
}

Matrix4 multiplyMatrix(const Matrix4& left, const Matrix4& right) {
    Matrix4 result{};
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            float value = 0.0f;
            for (int index = 0; index < 4; ++index) {
                value += left.values[row * 4 + index] * right.values[index * 4 + column];
            }
            result.values[row * 4 + column] = value;
        }
    }
    return result;
}

Matrix4 scaleMatrix(float x, float y, float z) {
    Matrix4 matrix = identityMatrix();
    matrix.values[0] = x;
    matrix.values[5] = y;
    matrix.values[10] = z;
    return matrix;
}

Matrix4 translationMatrix(float x, float y, float z) {
    Matrix4 matrix = identityMatrix();
    matrix.values[12] = x;
    matrix.values[13] = y;
    matrix.values[14] = z;
    return matrix;
}

Matrix4 yRotationMatrix(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    Matrix4 matrix = identityMatrix();
    matrix.values[0] = c;
    matrix.values[2] = -s;
    matrix.values[8] = s;
    matrix.values[10] = c;
    return matrix;
}

Matrix4 perspectiveMatrix(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane) {
    const float yScale = 1.0f / std::tan(verticalFovRadians * 0.5f);
    const float xScale = yScale / aspectRatio;
    const float zScale = farPlane / (farPlane - nearPlane);
    const float zOffset = (-nearPlane * farPlane) / (farPlane - nearPlane);

    Matrix4 matrix{};
    matrix.values[0] = xScale;
    matrix.values[5] = yScale;
    matrix.values[10] = zScale;
    matrix.values[11] = 1.0f;
    matrix.values[14] = zOffset;
    return matrix;
}

bool isCameraUsable(const RenderCamera& camera) {
    if (camera.fovy <= 0.0f) {
        return false;
    }

    const Float3 forward = subtract3({camera.target.x, camera.target.y, camera.target.z},
                                      {camera.position.x, camera.position.y, camera.position.z});
    if (length3(forward) < 1e-5f) {
        return false;
    }

    const Float3 up = {camera.up.x, camera.up.y, camera.up.z};
    if (length3(up) < 1e-5f) {
        return false;
    }

    return true;
}

void computeViewProjection(const RenderCamera& camera, int width, int height,
                           Matrix4& outView, Matrix4& outProjection) {
    const float aspectRatio = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

    if (isCameraUsable(camera)) {
        const Float3 eye = {camera.position.x, camera.position.y, camera.position.z};
        const Float3 target = {camera.target.x, camera.target.y, camera.target.z};
        const Float3 up = {camera.up.x, camera.up.y, camera.up.z};
        outView = lookAtMatrix(eye, target, up);
        outProjection = perspectiveMatrix(camera.fovy * Pi / 180.0f, aspectRatio, 0.1f, 100.0f);
    } else {
        // Fallback: fixed camera looking at origin from Z offset.
        outView = translationMatrix(0.0f, 0.0f, 5.0f);
        outProjection = perspectiveMatrix(65.0f * Pi / 180.0f, aspectRatio, 0.1f, 100.0f);
    }
}

bool isSupportedBoxBucket(RenderBucket bucket) {
    return bucket == RenderBucket::Opaque || bucket == RenderBucket::Vehicle ||
           bucket == RenderBucket::Decal || bucket == RenderBucket::Glass ||
           bucket == RenderBucket::Translucent || bucket == RenderBucket::Debug;
}

bool isAlphaBlendBucket(RenderBucket bucket) {
    return bucket == RenderBucket::Glass || bucket == RenderBucket::Translucent;
}

float colorChannel(std::uint8_t value) {
    return static_cast<float>(value) / 255.0f;
}

bool compileShader(const char* source,
                   const char* entryPoint,
                   const char* target,
                   ID3DBlob** shaderBlob,
                   std::string* error) {
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if !defined(NDEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* errorBlob = nullptr;
    const HRESULT hr = D3DCompile(source,
                                  std::strlen(source),
                                  "bs3d_d3d11_renderer_box",
                                  nullptr,
                                  nullptr,
                                  entryPoint,
                                  target,
                                  compileFlags,
                                  0,
                                  shaderBlob,
                                  &errorBlob);
    if (FAILED(hr)) {
        std::string errorText = "no compiler diagnostics";
        if (errorBlob != nullptr && errorBlob->GetBufferPointer() != nullptr) {
            errorText.assign(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        }
        releaseAndNull(errorBlob);
        assignError(error, std::string("D3DCompile failed for ") + entryPoint + " (" + target +
                               ") with HRESULT " + formatHresult(hr) + ": " + errorText);
        return false;
    }

    releaseAndNull(errorBlob);
    return true;
}

bool createPrimitivePipeline(ID3D11Device* device,
                             ID3D11VertexShader** vertexShader,
                             ID3D11PixelShader** pixelShader,
                             ID3D11InputLayout** inputLayout,
                             ID3D11Buffer** vertexBuffer,
                             ID3D11Buffer** indexBuffer,
                             ID3D11Buffer** constantBuffer,
                             std::string* error) {
    const char* shaderSource = R"(
struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_Position;
    float4 color : COLOR;
};

cbuffer PrimitiveConstants : register(b0) {
    row_major float4x4 mvp;
    float4 color;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.color = color;
    return output;
}

float4 PSMain(PSInput input) : SV_Target {
    return input.color;
}
)";

    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* pixelShaderBlob = nullptr;
    if (!compileShader(shaderSource, "VSMain", "vs_5_0", &vertexShaderBlob, error)) {
        return false;
    }
    if (!compileShader(shaderSource, "PSMain", "ps_5_0", &pixelShaderBlob, error)) {
        releaseAndNull(vertexShaderBlob);
        return false;
    }

    HRESULT hr = device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
                                            vertexShaderBlob->GetBufferSize(),
                                            nullptr,
                                            vertexShader);
    if (FAILED(hr)) {
        releaseAndNull(pixelShaderBlob);
        releaseAndNull(vertexShaderBlob);
        assignError(error, "ID3D11Device::CreateVertexShader failed with HRESULT " + formatHresult(hr));
        return false;
    }

    hr = device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
                                   pixelShaderBlob->GetBufferSize(),
                                   nullptr,
                                   pixelShader);
    if (FAILED(hr)) {
        releaseAndNull(pixelShaderBlob);
        releaseAndNull(vertexShaderBlob);
        assignError(error, "ID3D11Device::CreatePixelShader failed with HRESULT " + formatHresult(hr));
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device->CreateInputLayout(inputElements,
                                   static_cast<UINT>(std::size(inputElements)),
                                   vertexShaderBlob->GetBufferPointer(),
                                   vertexShaderBlob->GetBufferSize(),
                                   inputLayout);
    releaseAndNull(pixelShaderBlob);
    releaseAndNull(vertexShaderBlob);
    if (FAILED(hr)) {
        assignError(error, "ID3D11Device::CreateInputLayout failed with HRESULT " + formatHresult(hr));
        return false;
    }

    const Vertex vertices[] = {
        {{-0.5f, -0.5f, -0.5f}},
        {{-0.5f,  0.5f, -0.5f}},
        {{ 0.5f,  0.5f, -0.5f}},
        {{ 0.5f, -0.5f, -0.5f}},
        {{-0.5f, -0.5f,  0.5f}},
        {{-0.5f,  0.5f,  0.5f}},
        {{ 0.5f,  0.5f,  0.5f}},
        {{ 0.5f, -0.5f,  0.5f}},
    };

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(vertices));
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices;

    HRESULT bufferHr = device->CreateBuffer(&vertexBufferDesc, &vertexData, vertexBuffer);
    if (FAILED(bufferHr)) {
        assignError(error, "ID3D11Device::CreateBuffer failed for cube vertex buffer with HRESULT " +
                               formatHresult(bufferHr));
        return false;
    }

    const std::uint16_t indices[] = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7,
    };

    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(indices));
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = indices;

    bufferHr = device->CreateBuffer(&indexBufferDesc, &indexData, indexBuffer);
    if (FAILED(bufferHr)) {
        assignError(error, "ID3D11Device::CreateBuffer failed for cube index buffer with HRESULT " +
                               formatHresult(bufferHr));
        return false;
    }

    D3D11_BUFFER_DESC constantBufferDesc{};
    constantBufferDesc.ByteWidth = static_cast<UINT>(sizeof(PrimitiveConstants));
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    bufferHr = device->CreateBuffer(&constantBufferDesc, nullptr, constantBuffer);
    if (FAILED(bufferHr)) {
        assignError(error, "ID3D11Device::CreateBuffer failed for primitive constant buffer with HRESULT " +
                               formatHresult(bufferHr));
        return false;
    }

    return true;
}

bool createDebugLinePipeline(ID3D11Device* device,
                             ID3D11VertexShader** vertexShader,
                             ID3D11PixelShader** pixelShader,
                             ID3D11InputLayout** inputLayout,
                             ID3D11Buffer** vertexBuffer,
                             std::string* error) {
    const char* shaderSource = R"(
struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_Position;
    float4 color : COLOR;
};

cbuffer LineConstants : register(b0) {
    row_major float4x4 mvp;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_Target {
    return input.color;
}
)";

    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* pixelShaderBlob = nullptr;
    if (!compileShader(shaderSource, "VSMain", "vs_5_0", &vertexShaderBlob, error)) {
        return false;
    }
    if (!compileShader(shaderSource, "PSMain", "ps_5_0", &pixelShaderBlob, error)) {
        releaseAndNull(vertexShaderBlob);
        return false;
    }

    HRESULT hr = device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
                                            vertexShaderBlob->GetBufferSize(),
                                            nullptr,
                                            vertexShader);
    if (FAILED(hr)) {
        releaseAndNull(pixelShaderBlob);
        releaseAndNull(vertexShaderBlob);
        assignError(error, "ID3D11Device::CreateVertexShader failed for debug lines with HRESULT " +
                               formatHresult(hr));
        return false;
    }

    hr = device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
                                   pixelShaderBlob->GetBufferSize(),
                                   nullptr,
                                   pixelShader);
    if (FAILED(hr)) {
        releaseAndNull(pixelShaderBlob);
        releaseAndNull(vertexShaderBlob);
        assignError(error, "ID3D11Device::CreatePixelShader failed for debug lines with HRESULT " +
                               formatHresult(hr));
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device->CreateInputLayout(inputElements,
                                   static_cast<UINT>(std::size(inputElements)),
                                   vertexShaderBlob->GetBufferPointer(),
                                   vertexShaderBlob->GetBufferSize(),
                                   inputLayout);
    releaseAndNull(pixelShaderBlob);
    releaseAndNull(vertexShaderBlob);
    if (FAILED(hr)) {
        assignError(error, "ID3D11Device::CreateInputLayout failed for debug lines with HRESULT " +
                               formatHresult(hr));
        return false;
    }

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(LineVertex) * DebugLineVertexCapacity);
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = device->CreateBuffer(&vertexBufferDesc, nullptr, vertexBuffer);
    if (FAILED(hr)) {
        assignError(error, "ID3D11Device::CreateBuffer failed for debug line vertex buffer with HRESULT " +
                               formatHresult(hr));
        return false;
    }

    return true;
}

} // namespace

D3D11Renderer::~D3D11Renderer() {
    shutdown();
}

bool D3D11Renderer::initialize(const D3D11RendererConfig& config, std::string* error) {
    shutdown();

    if (config.window == nullptr) {
        assignError(error, "D3D11Renderer requires a valid HWND");
        return false;
    }
    if (config.width <= 0 || config.height <= 0) {
        assignError(error, "D3D11Renderer requires positive width and height");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferDesc.Width = static_cast<UINT>(config.width);
    swapChainDesc.BufferDesc.Height = static_cast<UINT>(config.height);
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = config.window;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (config.enableDebugLayer) {
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                               D3D_DRIVER_TYPE_HARDWARE,
                                               nullptr,
                                               deviceFlags,
                                               featureLevels,
                                               static_cast<UINT>(std::size(featureLevels)),
                                               D3D11_SDK_VERSION,
                                               &swapChainDesc,
                                               &swapChain_,
                                               &device_,
                                               &featureLevel,
                                               &context_);

    if (FAILED(hr) && config.enableDebugLayer) {
        shutdown();
        deviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                           D3D_DRIVER_TYPE_HARDWARE,
                                           nullptr,
                                           deviceFlags,
                                           featureLevels,
                                           static_cast<UINT>(std::size(featureLevels)),
                                           D3D11_SDK_VERSION,
                                           &swapChainDesc,
                                           &swapChain_,
                                           &device_,
                                           &featureLevel,
                                           &context_);
    }

    if (FAILED(hr)) {
        shutdown();
        assignError(error, "D3D11CreateDeviceAndSwapChain failed with HRESULT " + formatHresult(hr));
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "IDXGISwapChain::GetBuffer failed with HRESULT " + formatHresult(hr));
        return false;
    }

    hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    backBuffer->Release();
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "ID3D11Device::CreateRenderTargetView failed with HRESULT " + formatHresult(hr));
        return false;
    }

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = static_cast<UINT>(config.width);
    depthDesc.Height = static_cast<UINT>(config.height);
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = device_->CreateTexture2D(&depthDesc, nullptr, &depthStencilTexture_);
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "ID3D11Device::CreateTexture2D failed for depth stencil with HRESULT " +
                               formatHresult(hr));
        return false;
    }

    hr = device_->CreateDepthStencilView(depthStencilTexture_, nullptr, &depthStencilView_);
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "ID3D11Device::CreateDepthStencilView failed with HRESULT " + formatHresult(hr));
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    hr = device_->CreateDepthStencilState(&depthStencilDesc, &depthStencilState_);
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "ID3D11Device::CreateDepthStencilState failed with HRESULT " + formatHresult(hr));
        return false;
    }

    D3D11_BLEND_DESC opaqueBlendDesc{};
    opaqueBlendDesc.RenderTarget[0].BlendEnable = FALSE;
    opaqueBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device_->CreateBlendState(&opaqueBlendDesc, &opaqueBlendState_);
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "ID3D11Device::CreateBlendState failed for opaque blend with HRESULT " +
                               formatHresult(hr));
        return false;
    }

    D3D11_BLEND_DESC alphaBlendDesc{};
    alphaBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    alphaBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    alphaBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    alphaBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    alphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    alphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    alphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    alphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device_->CreateBlendState(&alphaBlendDesc, &alphaBlendState_);
    if (FAILED(hr)) {
        shutdown();
        assignError(error, "ID3D11Device::CreateBlendState failed for alpha blend with HRESULT " +
                               formatHresult(hr));
        return false;
    }

    if (!createPrimitivePipeline(device_,
                                 &vertexShader_,
                                 &pixelShader_,
                                 &inputLayout_,
                                 &vertexBuffer_,
                                 &indexBuffer_,
                                 &constantBuffer_,
                                 error)) {
        shutdown();
        return false;
    }

    if (!createDebugLinePipeline(device_,
                                 &lineVertexShader_,
                                 &linePixelShader_,
                                 &lineInputLayout_,
                                 &lineVertexBuffer_,
                                 error)) {
        shutdown();
        return false;
    }

    width_ = config.width;
    height_ = config.height;

    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool D3D11Renderer::isInitialized() const {
    return device_ != nullptr && context_ != nullptr && swapChain_ != nullptr && renderTargetView_ != nullptr &&
           depthStencilTexture_ != nullptr && depthStencilView_ != nullptr && depthStencilState_ != nullptr &&
           opaqueBlendState_ != nullptr && alphaBlendState_ != nullptr &&
           vertexShader_ != nullptr && pixelShader_ != nullptr && inputLayout_ != nullptr &&
           vertexBuffer_ != nullptr && indexBuffer_ != nullptr && constantBuffer_ != nullptr &&
           lineVertexShader_ != nullptr && linePixelShader_ != nullptr && lineInputLayout_ != nullptr &&
           lineVertexBuffer_ != nullptr;
}

void D3D11Renderer::shutdown() {
    releaseAndNull(lineVertexBuffer_);
    releaseAndNull(lineInputLayout_);
    releaseAndNull(linePixelShader_);
    releaseAndNull(lineVertexShader_);
    releaseAndNull(constantBuffer_);
    releaseAndNull(indexBuffer_);
    releaseAndNull(vertexBuffer_);
    releaseAndNull(inputLayout_);
    releaseAndNull(pixelShader_);
    releaseAndNull(vertexShader_);
    releaseAndNull(alphaBlendState_);
    releaseAndNull(opaqueBlendState_);
    releaseAndNull(depthStencilState_);
    releaseAndNull(depthStencilView_);
    releaseAndNull(depthStencilTexture_);
    releaseAndNull(renderTargetView_);
    releaseAndNull(swapChain_);
    releaseAndNull(context_);
    releaseAndNull(device_);
    width_ = 0;
    height_ = 0;
}

void D3D11Renderer::renderFrame(const RenderFrame& frame) {
    ++renderCalls_;
    lastStats_ = summarizeRenderFrame(frame);
    lastValidation_ = validateRenderFrame(frame);

    if (!isInitialized()) {
        return;
    }

    const float clearColor[] = {0.05f, 0.08f, 0.16f, 1.0f};
    ID3D11RenderTargetView* renderTargetView = renderTargetView_;
    context_->OMSetRenderTargets(1, &renderTargetView, depthStencilView_);
    context_->ClearRenderTargetView(renderTargetView_, clearColor);
    context_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH, 1.0f, 0);
    context_->OMSetDepthStencilState(depthStencilState_, 0);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    ID3D11Buffer* vertexBuffer = vertexBuffer_;
    context_->IASetInputLayout(inputLayout_);
    context_->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context_->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R16_UINT, 0);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertexShader_, nullptr, 0);
    context_->PSSetShader(pixelShader_, nullptr, 0);
    ID3D11Buffer* constantBuffer = constantBuffer_;
    context_->VSSetConstantBuffers(0, 1, &constantBuffer);
    context_->PSSetConstantBuffers(0, 1, &constantBuffer);

    Matrix4 view;
    Matrix4 projection;
    computeViewProjection(frame.camera, width_, height_, view, projection);

    bool usingAlphaBlend = false;
    context_->OMSetBlendState(opaqueBlendState_, nullptr, 0xFFFFFFFF);

    for (const RenderPrimitiveCommand& command : frame.primitives) {
        if (command.kind != RenderPrimitiveKind::Box || !isSupportedBoxBucket(command.bucket)) {
            continue;
        }

        if (isAlphaBlendBucket(command.bucket)) {
            if (!usingAlphaBlend) {
                context_->OMSetBlendState(alphaBlendState_, nullptr, 0xFFFFFFFF);
                usingAlphaBlend = true;
            }
        } else {
            if (usingAlphaBlend) {
                context_->OMSetBlendState(opaqueBlendState_, nullptr, 0xFFFFFFFF);
                usingAlphaBlend = false;
            }
        }

        PrimitiveConstants constants{};
        const Matrix4 scale = scaleMatrix(command.size.x * command.transform.scale.x,
                                          command.size.y * command.transform.scale.y,
                                          command.size.z * command.transform.scale.z);
        const Matrix4 rotation = yRotationMatrix(command.transform.yawRadians);
        const Matrix4 translation = translationMatrix(command.transform.position.x,
                                                      command.transform.position.y,
                                                      command.transform.position.z);
        const Matrix4 model = multiplyMatrix(multiplyMatrix(scale, rotation), translation);
        const Matrix4 mvp = multiplyMatrix(multiplyMatrix(model, view), projection);
        std::memcpy(constants.mvp, mvp.values, sizeof(constants.mvp));
        constants.color[0] = colorChannel(command.tint.r);
        constants.color[1] = colorChannel(command.tint.g);
        constants.color[2] = colorChannel(command.tint.b);
        constants.color[3] = colorChannel(command.tint.a);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        const HRESULT mapHr = context_->Map(constantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(mapHr)) {
            continue;
        }
        std::memcpy(mapped.pData, &constants, sizeof(constants));
        context_->Unmap(constantBuffer_, 0);
        context_->DrawIndexed(36, 0, 0);
    }

    if (usingAlphaBlend) {
        context_->OMSetBlendState(opaqueBlendState_, nullptr, 0xFFFFFFFF);
    }

    if (!frame.debugLines.empty()) {
        const UINT requestedLineCount = static_cast<UINT>(frame.debugLines.size());
        const UINT drawableLineCount = requestedLineCount > DebugLineVertexCapacity / 2
                                           ? DebugLineVertexCapacity / 2
                                           : requestedLineCount;
        if (drawableLineCount > 0) {
            D3D11_MAPPED_SUBRESOURCE mappedLines{};
            const HRESULT lineMapHr = context_->Map(lineVertexBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedLines);
            if (SUCCEEDED(lineMapHr)) {
                LineVertex* vertices = static_cast<LineVertex*>(mappedLines.pData);
                for (UINT index = 0; index < drawableLineCount; ++index) {
                    const RenderLineCommand& line = frame.debugLines[index];
                    const float r = colorChannel(line.tint.r);
                    const float g = colorChannel(line.tint.g);
                    const float b = colorChannel(line.tint.b);
                    const float a = colorChannel(line.tint.a);

                    vertices[index * 2] = {{line.start.x, line.start.y, line.start.z}, {r, g, b, a}};
                    vertices[index * 2 + 1] = {{line.end.x, line.end.y, line.end.z}, {r, g, b, a}};
                }
                context_->Unmap(lineVertexBuffer_, 0);

                PrimitiveConstants lineConstants{};
                const Matrix4 lineMvp = multiplyMatrix(view, projection);
                std::memcpy(lineConstants.mvp, lineMvp.values, sizeof(lineConstants.mvp));

                D3D11_MAPPED_SUBRESOURCE mappedConstants{};
                const HRESULT constantMapHr =
                    context_->Map(constantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedConstants);
                if (SUCCEEDED(constantMapHr)) {
                    std::memcpy(mappedConstants.pData, &lineConstants, sizeof(lineConstants));
                    context_->Unmap(constantBuffer_, 0);

                    const UINT lineStride = sizeof(LineVertex);
                    const UINT lineOffset = 0;
                    ID3D11Buffer* lineVertexBuffer = lineVertexBuffer_;
                    context_->IASetInputLayout(lineInputLayout_);
                    context_->IASetVertexBuffers(0, 1, &lineVertexBuffer, &lineStride, &lineOffset);
                    context_->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
                    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                    context_->VSSetShader(lineVertexShader_, nullptr, 0);
                    context_->PSSetShader(linePixelShader_, nullptr, 0);
                    ID3D11Buffer* constantBuffer = constantBuffer_;
                    context_->VSSetConstantBuffers(0, 1, &constantBuffer);
                    context_->Draw(drawableLineCount * 2, 0);
                }
            }
        }
    }

    swapChain_->Present(1, 0);
}

} // namespace bs3d
