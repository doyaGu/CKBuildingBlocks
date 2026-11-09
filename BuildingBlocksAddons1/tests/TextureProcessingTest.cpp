// Pixel kernels of Filter Texture, Blend Textures and Water Texture, checked
// against values worked out by hand from the original DLL's MMX/SSE2 code.
#include "CKAll.h"
#include "../TextureProcessing.h"

#include <cstdio>

static int g_Failures = 0;

static void ExpectPixels(const char *name, const CKDWORD *actual, const CKDWORD *expected, int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (actual[i] != expected[i])
        {
            std::fprintf(stderr, "%s: pixel %d is %08X, expected %08X\n", name, i,
                         static_cast<unsigned int>(actual[i]), static_cast<unsigned int>(expected[i]));
            ++g_Failures;
        }
    }
}

static CKDWORD Grey(int value)
{
    return static_cast<CKDWORD>(value) * 0x01010101u;
}

// Pads a width x height image the way ConvolveTexture3x3 does.
static void Pad(const CKDWORD *image, int width, int height, CKDWORD *padded)
{
    const int stride = width + 2;
    for (int y = -1; y <= height; ++y)
    {
        const int sy = y < 0 ? 0 : (y >= height ? height - 1 : y);
        for (int x = -1; x <= width; ++x)
        {
            const int sx = x < 0 ? 0 : (x >= width ? width - 1 : x);
            padded[(y + 1) * stride + (x + 1)] = image[sy * width + sx];
        }
    }
}

static void TestKernelData()
{
    // Uniform blur: weights are (1 << 8) / 9 = 28, truncated.
    int uniform[3][3] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
    short data[24];
    BuildConvolutionKernelData(uniform, 9, data);
    for (int i = 0; i < 24; ++i)
    {
        if (data[i] != 28)
        {
            std::fprintf(stderr, "kernel data: entry %d is %d, expected 28\n", i, data[i]);
            ++g_Failures;
        }
    }

    // Weights wrap to 16 bits: 200 << 8 = 51200 -> -14336.
    int kernel[3][3] = {{1, 2, 3}, {4, 200, 6}, {7, 8, 9}};
    BuildConvolutionKernelData(kernel, 1, data);
    const short expected[24] = {512, 256, 512, 256, 768, 768, 768, 768,
                                -14336, 1024, -14336, 1024, 1536, 1536, 1536, 1536,
                                2048, 1792, 2048, 1792, 2304, 2304, 2304, 2304};
    for (int i = 0; i < 24; ++i)
    {
        if (data[i] != expected[i])
        {
            std::fprintf(stderr, "kernel data: entry %d is %d, expected %d\n", i, data[i], expected[i]);
            ++g_Failures;
        }
    }
}

static void TestFilterShear()
{
    // Identity kernel on a 2x2 image A B / C D. The DLL's row loop only skips
    // one padding pixel, so the second row reads padded[2][0..1] = C C.
    const CKDWORD image[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    CKDWORD padded[16];
    Pad(image, 2, 2, padded);

    int identity[3][3] = {{0, 0, 0}, {0, 1, 0}, {0, 0, 0}};
    short data[24];
    BuildConvolutionKernelData(identity, 1, data);

    CKDWORD out[4] = {0, 0, 0, 0};
    ConvolvePaddedPixels3x3(padded, out, 2, 2, data);
    const CKDWORD expected[4] = {0x11111111, 0x22222222, 0x33333333, 0x33333333};
    ExpectPixels("filter shear", out, expected, 4);
}

static void TestFilterWeights()
{
    // Uniform blur of a white 1x1 image: 9 * 28 * 255 >> 8 = 251 per lane.
    const CKDWORD white = 0xFFFFFFFF;
    CKDWORD padded[9];
    Pad(&white, 1, 1, padded);
    int uniform[3][3] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
    short data[24];
    BuildConvolutionKernelData(uniform, 9, data);
    CKDWORD out = 0;
    ConvolvePaddedPixels3x3(padded, &out, 1, 1, data);
    const CKDWORD expectedBlur = Grey(251);
    ExpectPixels("filter uniform blur", &out, &expectedBlur, 1);

    // Row 0 of the matrix weighs the row above, column 0 the left pixel.
    // Kernel: top-centre 2, centre-left 1, scale 1.
    const CKDWORD image[9] = {0, 0x00000020, 0, 0x00000010, 0x00400040, 0x00000000, 0, 0, 0};
    CKDWORD padded3[25];
    Pad(image, 3, 3, padded3);
    int kernel[3][3] = {{0, 2, 0}, {1, 0, 0}, {0, 0, 0}};
    BuildConvolutionKernelData(kernel, 1, data);
    CKDWORD out3[9];
    ConvolvePaddedPixels3x3(padded3, out3, 3, 3, data);
    // Row 0 (the padding row above repeats image row 0):
    // (0,0) = 0; (1,0) = 2 * 0x20 + 0 = 0x40; (2,0) = 0 + 0x20 = 0x20.
    const CKDWORD expectedRow0[3] = {0x00000000, 0x00000040, 0x00000020};
    ExpectPixels("filter kernel orientation", out3, expectedRow0, 3);

    // Saturation: centre 5 * 0x40 = 0x140 -> 0xFF; a negative sum -> 0.
    int bright[3][3] = {{0, 0, 0}, {0, 5, 0}, {0, 0, 0}};
    BuildConvolutionKernelData(bright, 1, data);
    const CKDWORD single = 0x00400001;
    CKDWORD padded1[9];
    Pad(&single, 1, 1, padded1);
    ConvolvePaddedPixels3x3(padded1, &out, 1, 1, data);
    const CKDWORD expectedBright = 0x00FF0005;
    ExpectPixels("filter saturation", &out, &expectedBright, 1);

    int negative[3][3] = {{0, 0, 0}, {0, -1, 0}, {0, 0, 0}};
    BuildConvolutionKernelData(negative, 1, data);
    ConvolvePaddedPixels3x3(padded1, &out, 1, 1, data);
    const CKDWORD expectedZero = 0;
    ExpectPixels("filter negative", &out, &expectedZero, 1);
}

static void TestBlend()
{
    const CKDWORD first[3] = {Grey(255), Grey(255), 0x00FF0000};
    const CKDWORD second[3] = {Grey(1), Grey(255), 0x000000FF};
    CKDWORD out[3];

    // 0.5: (255 * 128 + 1 * 128) >> 8 = 128; (255 * 128 * 2) >> 8 = 255.
    BlendPixelBuffers(out, first, second, 3, 0.5f);
    const CKDWORD expectedHalf[3] = {Grey(128), Grey(255), 0x007F007F};
    ExpectPixels("blend 0.5", out, expectedHalf, 3);

    // 0.3f: weights 76 and 179 (truncated), so white + white = 255 * 255 >> 8 = 254.
    BlendPixelBuffers(out, first, second, 3, 0.3f);
    const CKDWORD expectedThird[3] = {Grey((255 * 179 + 76) >> 8), Grey(254), 0x00B2004B};
    ExpectPixels("blend 0.3", out, expectedThird, 3);

    // -1.0f: second weight -256 -> lanes 0xFF00 (B, R) and 0xFFFF (G, A);
    // 3 * 0xFF00 -> 0xFD00 and 3 * 0xFFFF -> 0xFFFD after pmullw.
    const CKDWORD zero = 0;
    const CKDWORD three = Grey(3);
    BlendPixelBuffers(out, &zero, &three, 1, -1.0f);
    const CKDWORD expectedNegative = 0xFFFDFFFD;
    ExpectPixels("blend -1", out, &expectedNegative, 1);
}

static void TestWater()
{
    // 3x3, border 0, previous frame 0, damping 4. Blue carries the values,
    // alpha is 255 everywhere and saturates.
    const int values[9] = {10, 20, 30, 40, 50, 60, 70, 80, 90};
    CKDWORD current[9];
    CKDWORD previous[9];
    for (int i = 0; i < 9; ++i)
    {
        current[i] = 0xFF000000u | static_cast<CKDWORD>(values[i]);
        previous[i] = 0;
    }
    ApplyWaterRipple(current, previous, 3, 3, 0xFF000000u, 4);
    // Top and bottom rows: (l + r + u + d) >> 2. Middle row: pavgb-style.
    const int expectedBlue[9] = {29, 42, 38, 60, 94, 79, 57, 98, 66};
    CKDWORD expected[9];
    for (int i = 0; i < 9; ++i)
        expected[i] = 0xFF000000u | static_cast<CKDWORD>(expectedBlue[i]);
    ExpectPixels("water 3x3", previous, expected, 9);

    // Middle-row rounding: u = 1, d = 0, l = 1, r = 0 -> s = 1 -> 2 - 0 = 2.
    CKDWORD rounding[9] = {0, 1, 0, 1, 9, 0, 0, 0, 0};
    CKDWORD out[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    ApplyWaterRipple(rounding, out, 3, 3, 0, 4);
    if ((out[4] & 0xFF) != 2)
    {
        std::fprintf(stderr, "water rounding: centre is %u, expected 2\n", static_cast<unsigned int>(out[4] & 0xFF));
        ++g_Failures;
    }

    // Damping range: 0 zeroes everything, 16 and negative values disable it;
    // the result is clamped to 255 after damping.
    CKDWORD flat[9];
    for (int i = 0; i < 9; ++i)
        flat[i] = Grey(200);
    const int dampings[4] = {8, 0, 16, -1};
    const CKDWORD results[4] = {Grey(255), 0, Grey(255), Grey(255)};
    for (int k = 0; k < 4; ++k)
    {
        CKDWORD prev[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
        ApplyWaterRipple(flat, prev, 3, 3, Grey(200), dampings[k]);
        CKDWORD expect[9];
        for (int i = 0; i < 9; ++i)
            expect[i] = results[k];
        ExpectPixels("water damping range", prev, expect, 9);
    }

    // 400 - 255 = 145; damping 1 -> 145 - 72 = 73. Old above 2s -> 0.
    CKDWORD prev[9];
    for (int i = 0; i < 9; ++i)
        prev[i] = Grey(255);
    ApplyWaterRipple(flat, prev, 3, 3, Grey(200), 1);
    CKDWORD expect73[9];
    for (int i = 0; i < 9; ++i)
        expect73[i] = Grey(73);
    ExpectPixels("water previous frame", prev, expect73, 9);

    CKDWORD low[9];
    for (int i = 0; i < 9; ++i)
    {
        low[i] = Grey(10);
        prev[i] = Grey(30);
    }
    ApplyWaterRipple(low, prev, 3, 3, Grey(10), 4);
    CKDWORD expectZero[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    ExpectPixels("water underflow", prev, expectZero, 9);
}

int main()
{
    TestKernelData();
    TestFilterShear();
    TestFilterWeights();
    TestBlend();
    TestWater();
    if (g_Failures)
    {
        std::fprintf(stderr, "%d failure(s)\n", g_Failures);
        return 1;
    }
    std::printf("TextureProcessingTest passed\n");
    return 0;
}
