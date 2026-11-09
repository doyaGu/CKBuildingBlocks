#ifndef TEXTUREPROCESSING_H
#define TEXTUREPROCESSING_H

//////////////////////////////////////
// Texture Processing functions

// Convolutions Matrix
// Gaussian Blur Matrix
extern int CMAntialias[3][3];
// Gaussian Blur Matrix
extern int CMGaussianBlur[3][3];
// Gaussian Blur Matrix
extern int CMUniformBlur[3][3];
// Gaussian Blur Matrix
extern int CMDirectionnal[3][3];
// Emboss Matrix
extern int CMEmboss[3][3];
// Passe haut Matrix
extern int CMHighPass[3][3];
// Passe bas Matrix
extern int CMLowPass[3][3];

// Decal  Matrices
extern int CMDecalLeft[3][3];
extern int CMDecalRight[3][3];
extern int CMDecalTop[3][3];
extern int CMDecalBottom[3][3];
extern int CMDecalNone[3][3];
extern int CMDecalBR[3][3];
extern int CMDecalBL[3][3];
extern int CMDecalTL[3][3];
extern int CMDecalTR[3][3];

void ConvolveTexture3x3(CKTexture *texture, int kernel[3][3], int scale);

// Pixel kernels matching the integer MMX/SSE2 code of the original DLL.
// Builds the 24-entry pmaddwd weight table from a 3x3 kernel and a non-zero scale.
void BuildConvolutionKernelData(int kernel[3][3], int scale, short kernelData[24]);
// paddedSource is (width + 2) x (height + 2) with clamped borders; writes width x height.
void ConvolvePaddedPixels3x3(const CKDWORD *paddedSource,
                             CKDWORD *destination,
                             int width,
                             int height,
                             const short *kernelData);
// Water Texture step: writes the new frame over previousFrame.
void ApplyWaterRipple(CKDWORD *currentFrame,
                      CKDWORD *previousFrame,
                      int width,
                      int height,
                      CKDWORD borderColor,
                      int damping);
// Blend Textures: output = first * (1 - blendFactor) + second * blendFactor.
void BlendPixelBuffers(CKDWORD *output,
                       const CKDWORD *first,
                       const CKDWORD *second,
                       int pixelCount,
                       float blendFactor);

#endif // TEXTUREPROCESSING_H
