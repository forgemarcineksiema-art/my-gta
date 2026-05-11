#include "D3D11BootPipeline.h"

#include <d3dcompiler.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace d3d11_spike {

namespace {

struct Matrix4 {
    float values[16];
};

struct alignas(16) TransformConstants {
    // CPU matrices are row-major and consumed by HLSL as mul(row_vector, row_major_matrix).
    float mvp[16];
};

static_assert(sizeof(TransformConstants) % 16 == 0, "D3D11 constant buffers must be 16-byte aligned");

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

Matrix4 translationMatrix(float x, float y, float z) {
    Matrix4 matrix = identityMatrix();
    matrix.values[12] = x;
    matrix.values[13] = y;
    matrix.values[14] = z;
    return matrix;
}

Matrix4 xRotationMatrix(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    Matrix4 matrix = identityMatrix();
    matrix.values[5] = c;
    matrix.values[6] = s;
    matrix.values[9] = -s;
    matrix.values[10] = c;
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

ComPtr<ID3DBlob> compileShader(const char* source, const char* entryPoint, const char* target) {
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if !defined(NDEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    const HRESULT hr = D3DCompile(source,
                                  std::strlen(source),
                                  "bs3d_d3d11_boot_cube",
                                  nullptr,
                                  nullptr,
                                  entryPoint,
                                  target,
                                  compileFlags,
                                  0,
                                  shaderBlob.GetAddressOf(),
                                  errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        std::string errorText = "no compiler diagnostics";
        if (errorBlob.Get() != nullptr && errorBlob->GetBufferPointer() != nullptr) {
            errorText.assign(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        }
        throw std::runtime_error(std::string("D3DCompile failed for ") + entryPoint + " (" + target +
                                 ") with HRESULT " + formatHresult(hr) + ": " + errorText);
    }

    return shaderBlob;
}

} // namespace

void createCubePipeline(D3D11State& state) {
    const char* shaderSource = R"(
struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_Position;
    float4 color : COLOR;
};

cbuffer TransformConstants : register(b0) {
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

    ComPtr<ID3DBlob> vertexShaderBlob = compileShader(shaderSource, "VSMain", "vs_5_0");
    ComPtr<ID3DBlob> pixelShaderBlob = compileShader(shaderSource, "PSMain", "ps_5_0");

    HRESULT hr = state.device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
                                                  vertexShaderBlob->GetBufferSize(),
                                                  nullptr,
                                                  state.vertexShader.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateVertexShader failed with HRESULT " + formatHresult(hr));
    }

    hr = state.device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
                                         pixelShaderBlob->GetBufferSize(),
                                         nullptr,
                                         state.pixelShader.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreatePixelShader failed with HRESULT " + formatHresult(hr));
    }

    const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = state.device->CreateInputLayout(inputElements,
                                         static_cast<UINT>(std::size(inputElements)),
                                         vertexShaderBlob->GetBufferPointer(),
                                         vertexShaderBlob->GetBufferSize(),
                                         state.inputLayout.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateInputLayout failed with HRESULT " + formatHresult(hr));
    }

    const Vertex vertices[] = {
        {{-0.6f, -0.6f, -0.6f}, {1.0f, 0.10f, 0.08f, 1.0f}},
        {{-0.6f,  0.6f, -0.6f}, {1.0f, 0.72f, 0.10f, 1.0f}},
        {{ 0.6f,  0.6f, -0.6f}, {0.12f, 0.86f, 0.20f, 1.0f}},
        {{ 0.6f, -0.6f, -0.6f}, {0.10f, 0.45f, 1.0f, 1.0f}},
        {{-0.6f, -0.6f,  0.6f}, {0.90f, 0.12f, 1.0f, 1.0f}},
        {{-0.6f,  0.6f,  0.6f}, {0.20f, 0.95f, 0.95f, 1.0f}},
        {{ 0.6f,  0.6f,  0.6f}, {0.92f, 0.92f, 0.92f, 1.0f}},
        {{ 0.6f, -0.6f,  0.6f}, {0.98f, 0.45f, 0.12f, 1.0f}},
    };

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(vertices));
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices;

    hr = state.device->CreateBuffer(&vertexBufferDesc, &vertexData, state.vertexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateBuffer failed for cube vertex buffer with HRESULT " +
                                 formatHresult(hr));
    }

    const uint16_t indices[] = {
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

    hr = state.device->CreateBuffer(&indexBufferDesc, &indexData, state.indexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateBuffer failed for cube index buffer with HRESULT " +
                                 formatHresult(hr));
    }

    D3D11_BUFFER_DESC constantBufferDesc{};
    constantBufferDesc.ByteWidth = static_cast<UINT>(sizeof(TransformConstants));
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = state.device->CreateBuffer(&constantBufferDesc, nullptr, state.constantBuffer.GetAddressOf());
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11Device::CreateBuffer failed for constant buffer with HRESULT " +
                                 formatHresult(hr));
    }

    std::cout << "indexed cube pipeline created\n";
    std::cout << "constant buffer created\n";
}

void updateTransformConstants(D3D11State& state, int frameIndex, float aspectRatio) {
    TransformConstants constants{};
    constexpr float pi = 3.14159265358979323846f;
    const float frame = static_cast<float>(frameIndex);
    const Matrix4 model =
        multiplyMatrix(xRotationMatrix(0.45f + frame * 0.025f), yRotationMatrix(frame * 0.055f));
    const Matrix4 view = translationMatrix(0.0f, 0.0f, 4.0f);
    const Matrix4 projection = perspectiveMatrix(65.0f * pi / 180.0f, aspectRatio, 0.1f, 100.0f);
    const Matrix4 mvp = multiplyMatrix(multiplyMatrix(model, view), projection);
    std::memcpy(constants.mvp, mvp.values, sizeof(constants.mvp));

    D3D11_MAPPED_SUBRESOURCE mapped{};
    const HRESULT hr = state.context->Map(state.constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        throw std::runtime_error("ID3D11DeviceContext::Map failed for constant buffer with HRESULT " +
                                 formatHresult(hr));
    }

    std::memcpy(mapped.pData, &constants, sizeof(constants));
    state.context->Unmap(state.constantBuffer.Get(), 0);
}

} // namespace d3d11_spike
