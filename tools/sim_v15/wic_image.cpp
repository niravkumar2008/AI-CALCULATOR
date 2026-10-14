// WIC picture loading and PNG saving (see wic_image.h). COM is initialised by the caller
// (CoInitializeEx in main).
#include "wic_image.h"

#include <windows.h>
#include <wincodec.h>

#include <algorithm>

namespace {

template <class T>
struct Com {  // releases a COM pointer when it goes out of scope
  T* p = nullptr;
  ~Com() {
    if (p) p->Release();
  }
  T** operator&() { return &p; }
  T* operator->() { return p; }
};

bool factory(Com<IWICImagingFactory>& f) {
  return SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
                                    reinterpret_cast<void**>(&f)));
}

}  // namespace

bool wicLoadCover(const std::wstring& path, int w, int h, std::vector<uint8_t>& rgb) {
  Com<IWICImagingFactory> f;
  Com<IWICBitmapDecoder> dec;
  Com<IWICBitmapFrameDecode> frame;
  if (!factory(f) ||
      FAILED(f->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &dec)) ||
      FAILED(dec->GetFrame(0, &frame)))
    return false;
  UINT sw = 0, sh = 0;
  if (FAILED(frame->GetSize(&sw, &sh)) || sw == 0 || sh == 0) return false;
  const double scale = std::max(double(w) / sw, double(h) / sh);
  const UINT nw = std::max<UINT>(UINT(w), UINT(sw * scale + 0.5));
  const UINT nh = std::max<UINT>(UINT(h), UINT(sh * scale + 0.5));
  Com<IWICBitmapScaler> sc;
  Com<IWICFormatConverter> conv;
  if (FAILED(f->CreateBitmapScaler(&sc)) || FAILED(sc->Initialize(frame.p, nw, nh, WICBitmapInterpolationModeFant)) ||
      FAILED(f->CreateFormatConverter(&conv)) ||
      FAILED(conv->Initialize(sc.p, GUID_WICPixelFormat32bppBGR, WICBitmapDitherTypeNone, nullptr, 0,
                              WICBitmapPaletteTypeCustom)))
    return false;
  std::vector<uint8_t> px(size_t(w) * h * 4);
  WICRect r = {INT((nw - w) / 2), INT((nh - h) / 2), w, h};
  if (FAILED(conv->CopyPixels(&r, UINT(w * 4), UINT(px.size()), px.data()))) return false;
  rgb.resize(size_t(w) * h * 3);
  for (size_t i = 0; i < size_t(w) * h; ++i) {
    rgb[i * 3 + 0] = px[i * 4 + 2];
    rgb[i * 3 + 1] = px[i * 4 + 1];
    rgb[i * 3 + 2] = px[i * 4 + 0];
  }
  return true;
}

bool wicSavePng(const std::wstring& path, const uint32_t* bgrx, int w, int h) {
  Com<IWICImagingFactory> f;
  Com<IWICStream> st;
  Com<IWICBitmapEncoder> enc;
  Com<IWICBitmapFrameEncode> fe;
  Com<IPropertyBag2> props;
  if (!factory(f) || FAILED(f->CreateStream(&st)) || FAILED(st->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) ||
      FAILED(f->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc)) ||
      FAILED(enc->Initialize(st.p, WICBitmapEncoderNoCache)) || FAILED(enc->CreateNewFrame(&fe, &props)) ||
      FAILED(fe->Initialize(props.p)) || FAILED(fe->SetSize(UINT(w), UINT(h))))
    return false;
  WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGR;
  if (FAILED(fe->SetPixelFormat(&fmt))) return false;
  if (fmt != GUID_WICPixelFormat32bppBGR) {  // the encoder wants another layout: convert
    Com<IWICBitmap> bmp;
    Com<IWICFormatConverter> conv;
    if (FAILED(f->CreateBitmapFromMemory(UINT(w), UINT(h), GUID_WICPixelFormat32bppBGR, UINT(w * 4), UINT(w * h * 4),
                                         reinterpret_cast<BYTE*>(const_cast<uint32_t*>(bgrx)), &bmp)) ||
        FAILED(f->CreateFormatConverter(&conv)) ||
        FAILED(conv->Initialize(bmp.p, fmt, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom)) ||
        FAILED(fe->WriteSource(conv.p, nullptr)))
      return false;
  } else if (FAILED(fe->WritePixels(UINT(h), UINT(w * 4), UINT(w * h * 4),
                                    reinterpret_cast<BYTE*>(const_cast<uint32_t*>(bgrx))))) {
    return false;
  }
  return SUCCEEDED(fe->Commit()) && SUCCEEDED(enc->Commit());
}
