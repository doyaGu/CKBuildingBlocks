/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//              TextureProcessing
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
#include "TextureProcessing.h"

#include <string.h>

// Convolutions Matrix
// Gaussian Blur Matrix
int CMAntialias[3][3] = {1, 2, 1, 2, 8, 2, 1, 2, 1};
// Gaussian Blur Matrix
int CMGaussianBlur[3][3] = {2, 4, 2, 4, 0, 4, 2, 4, 2};
// Gaussian Blur Matrix
int CMUniformBlur[3][3] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
// Gaussian Blur Matrix
int CMDirectionnal[3][3] = {4, 3, 2, 3, 2, 2, 2, 1, 0};
// Emboss Matrix
int CMEmboss[3][3] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
// Passe haut Matrix
int CMHighPass[3][3] = {0, -1, 0, -1, 5, -1, 0, -1, 0};
// Passe bas Matrix
int CMLowPass[3][3] = {0, 1, 0, 1, 3, 1, 0, 1, 0};

// Decal Matrices
int CMDecalLeft[3][3] = {0, 0, 0, 1, 0, 0, 0, 0, 0};
int CMDecalRight[3][3] = {0, 0, 0, 0, 0, 1, 0, 0, 0};
int CMDecalTop[3][3] = {0, 0, 0, 0, 0, 0, 0, 1, 0};
int CMDecalBottom[3][3] = {0, 1, 0, 0, 0, 0, 0, 0, 0};
int CMDecalNone[3][3] = {0, 0, 0, 0, 1, 0, 0, 0, 0};
int CMDecalBR[3][3] = {1, 0, 0, 0, 0, 0, 0, 0, 0};
int CMDecalBL[3][3] = {0, 0, 1, 0, 0, 0, 0, 0, 0};
int CMDecalTL[3][3] = {0, 0, 0, 0, 0, 0, 0, 0, 1};
int CMDecalTR[3][3] = {0, 0, 0, 0, 0, 0, 1, 0, 0};

CKDWORD *g_Buffer = NULL;
int g_Size = 0;

// The kernels below reproduce the integer MMX/SSE2 paths of the original
// BuildingBlocksAddons1.dll byte for byte. Pixels are handled as four
// independent 8-bit lanes (B, G, R, A from the low byte up); alpha is not
// special-cased by any of them.

static inline int PixelLane(CKDWORD pixel, int lane)
{
    return static_cast<int>((pixel >> (lane * 8)) & 0xFF);
}

static inline int ClampToByte(int value)
{
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return value;
}

// Arithmetic shift right by 8 (psrad 8): rounds towards negative infinity.
static inline int ShiftRightSigned8(int value)
{
    if (value >= 0)
        return value >> 8;
    return -(((-value) + 255) >> 8);
}

void BuildConvolutionKernelData(int kernel[3][3], int scale, short kernelData[24])
{
    // The DLL stores (k << 8) / scale truncated to 16 bits.
    short n[3][3];
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b)
            n[a][b] = static_cast<short>(static_cast<int>(static_cast<unsigned int>(kernel[a][b]) << 8) / scale);

    // pmaddwd layout: (centre, left) pairs for the two-pixel load, then the
    // right-hand weight broadcast to the four lanes.
    for (int row = 0; row < 3; ++row)
    {
        short *dst = kernelData + row * 8;
        dst[0] = n[row][1];
        dst[1] = n[row][0];
        dst[2] = n[row][1];
        dst[3] = n[row][0];
        dst[4] = n[row][2];
        dst[5] = n[row][2];
        dst[6] = n[row][2];
        dst[7] = n[row][2];
    }
}

void ConvolveTexture3x3(CKTexture *texture, int kernel[3][3], int scale)
{
    if (!texture)
        return;

    if (!scale)
    {
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b)
                scale += kernel[a][b];
        if (!scale)
            scale = 1;
    }

    const int width = texture->GetWidth();
    const int height = texture->GetHeight();

    CKDWORD *image = reinterpret_cast<CKDWORD *>(texture->LockSurfacePtr());
    if (!image)
        return;
    if (width <= 0 || height <= 0)
    {
        texture->ReleaseSurfacePtr();
        return;
    }

    short kernelData[24];
    BuildConvolutionKernelData(kernel, scale, kernelData);

    const int paddedSize = (width + 2) * (height + 2);
    const int dstSize = width * height;

    if (!g_Buffer || paddedSize > g_Size)
    {
        if (g_Buffer)
            delete[] g_Buffer;
        g_Buffer = new CKDWORD[paddedSize];
        g_Size = paddedSize;
    }

    CKDWORD *padded = g_Buffer;
    const int paddedStride = width + 2;
    const int rowBytes = width * static_cast<int>(sizeof(CKDWORD));

    for (int y = 0; y < height; ++y)
    {
        const CKDWORD *srcRow = image + y * width;
        CKDWORD *dstRow = padded + (y + 1) * paddedStride + 1;
        dstRow[-1] = srcRow[0];
        dstRow[width] = srcRow[width - 1];
        memcpy(dstRow, srcRow, rowBytes);
    }

    memcpy(padded + 1, image, rowBytes);
    memcpy(padded + (height + 1) * paddedStride + 1, image + (height - 1) * width, rowBytes);

    padded[0] = image[0];
    padded[width + 1] = image[width - 1];
    padded[paddedSize - 1] = image[dstSize - 1];
    padded[paddedSize - paddedStride] = image[dstSize - width];

    ConvolvePaddedPixels3x3(padded, image, width, height, kernelData);
    texture->ReleaseSurfacePtr();
}

void ConvolvePaddedPixels3x3(const CKDWORD *paddedSource, CKDWORD *destination, int width, int height, const short *kernelData)
{
    if (!paddedSource || !destination || !kernelData || width <= 0 || height <= 0)
        return;

    // Weight for (row, column) of the 3x3 neighbourhood.
    int weight[3][3];
    for (int row = 0; row < 3; ++row)
    {
        weight[row][0] = kernelData[row * 8 + 1];
        weight[row][1] = kernelData[row * 8 + 0];
        weight[row][2] = kernelData[row * 8 + 4];
    }

    const int stride = width + 2;
    // The DLL's MMX loop only advances one extra pixel at the end of each row,
    // not the two padding pixels, so output row y samples the padded image
    // y pixels to the left of where it should (a one-pixel-per-row shear).
    const CKDWORD *centre = paddedSource + stride + 1;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x, ++centre)
        {
            int sum[4] = {0, 0, 0, 0};
            for (int row = 0; row < 3; ++row)
            {
                const CKDWORD *src = centre + (row - 1) * stride;
                for (int column = 0; column < 3; ++column)
                {
                    const CKDWORD pixel = src[column - 1];
                    const int w = weight[row][column];
                    for (int lane = 0; lane < 4; ++lane)
                        sum[lane] += PixelLane(pixel, lane) * w;
                }
            }

            CKDWORD out = 0;
            for (int lane = 0; lane < 4; ++lane)
                out |= static_cast<CKDWORD>(ClampToByte(ShiftRightSigned8(sum[lane]))) << (lane * 8);
            *destination++ = out;
        }
        ++centre;
    }
}

static CKDWORD BlendWeightBits(double value)
{
    // _ftol truncates to 64 bits (0x8000000000000000 when out of range) and
    // the DLL keeps the low 32 bits.
    if (!(value > -9.2e18 && value < 9.2e18))
        return 0;
    return static_cast<CKDWORD>(static_cast<unsigned long long>(static_cast<long long>(value)));
}

void BlendPixelBuffers(CKDWORD *output, const CKDWORD *first, const CKDWORD *second, int pixelCount, float blendFactor)
{
    if (!output || !first || !second || pixelCount <= 0)
        return;

    // fld factor / fmul 256.0f and fld1 / fsub factor / fmul 256.0f on the
    // x87 stack, which runs at double precision.
    const double f = static_cast<double>(blendFactor);
    const CKDWORD second32 = BlendWeightBits(f * 256.0);
    const CKDWORD first32 = BlendWeightBits((1.0 - f) * 256.0);

    // The 32-bit weight w is spread as (w << 16) | w over two 16-bit lanes and
    // duplicated, so B and R use lane 0 and G and A use lane 1.
    unsigned int firstWeight[4];
    unsigned int secondWeight[4];
    const CKDWORD firstPacked = first32 | (first32 << 16);
    const CKDWORD secondPacked = second32 | (second32 << 16);
    for (int lane = 0; lane < 4; ++lane)
    {
        firstWeight[lane] = (firstPacked >> ((lane & 1) * 16)) & 0xFFFF;
        secondWeight[lane] = (secondPacked >> ((lane & 1) * 16)) & 0xFFFF;
    }

    for (int i = 0; i < pixelCount; ++i)
    {
        const CKDWORD c0 = first[i];
        const CKDWORD c1 = second[i];
        CKDWORD out = 0;
        for (int lane = 0; lane < 4; ++lane)
        {
            // pmullw keeps the low 16 bits, paddusw saturates, psrlw 8.
            const unsigned int a = (PixelLane(c0, lane) * firstWeight[lane]) & 0xFFFF;
            const unsigned int b = (PixelLane(c1, lane) * secondWeight[lane]) & 0xFFFF;
            unsigned int sum = a + b;
            if (sum > 0xFFFF)
                sum = 0xFFFF;
            out |= static_cast<CKDWORD>(sum >> 8) << (lane * 8);
        }
        output[i] = out;
    }
}

static inline CKDWORD WaterSample(const CKDWORD *frame, int width, int height, int x, int y, CKDWORD borderColor)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return borderColor;
    return frame[y * width + x];
}

void ApplyWaterRipple(CKDWORD *currentFrame, CKDWORD *previousFrame, int width, int height, CKDWORD borderColor, int damping)
{
    if (!currentFrame || !previousFrame || width <= 0 || height <= 0)
        return;

    // psrlw takes the zero-extended 32-bit damping as its count: any count
    // above 15 (including negative values) shifts everything out.
    const unsigned int shift = static_cast<unsigned int>(damping);

    for (int y = 0; y < height; ++y)
    {
        // The SSE2 path handles the first and last rows with MMX code that
        // averages the four neighbours as (l + r + u + d) >> 2; the rows in
        // between use pavgb twice: ((u, d) avg + (l, r) avg) >> 1.
        const bool edgeRow = (y == 0 || y == height - 1);
        for (int x = 0; x < width; ++x)
        {
            const CKDWORD left = WaterSample(currentFrame, width, height, x - 1, y, borderColor);
            const CKDWORD right = WaterSample(currentFrame, width, height, x + 1, y, borderColor);
            const CKDWORD up = WaterSample(currentFrame, width, height, x, y - 1, borderColor);
            const CKDWORD down = WaterSample(currentFrame, width, height, x, y + 1, borderColor);
            const CKDWORD old = previousFrame[y * width + x];

            CKDWORD out = 0;
            for (int lane = 0; lane < 4; ++lane)
            {
                const int l = PixelLane(left, lane);
                const int r = PixelLane(right, lane);
                const int u = PixelLane(up, lane);
                const int d = PixelLane(down, lane);

                int s;
                if (edgeRow)
                    s = (l + r + u + d) >> 2;
                else
                    s = (((u + d + 1) >> 1) + ((l + r + 1) >> 1)) >> 1;

                int t = 2 * s - PixelLane(old, lane);
                if (t < 0)
                    t = 0;
                if (shift < 16)
                    t -= t >> shift;
                out |= static_cast<CKDWORD>(ClampToByte(t)) << (lane * 8);
            }
            previousFrame[y * width + x] = out;
        }
    }
}
