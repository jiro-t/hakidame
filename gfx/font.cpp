#include "font.hpp"
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#include <vector>
#include <DirectXMath.h>

namespace ino::shape
{

d3d::StaticMesh CreateCharMesh(wchar_t c, LPCWSTR font)
{
	namespace DX = DirectX;

	const int fontsize = 128;
	LOGFONT lf = { fontsize, 0, 0, 0, 0, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN, L"ＭＳ 明朝" };
	HFONT hFont;
	hFont = CreateFontIndirect(&lf);

	HDC hdc = GetDC(NULL);
	HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

	// フォントビットマップ取得
	TEXTMETRIC TM;
	GetTextMetrics(hdc, &TM);
	GLYPHMETRICS GM;
	CONST MAT2 Mat = { {0,1},{0,0},{0,0},{0,1} };
	DWORD size = GetGlyphOutline(hdc, c, GGO_GRAY4_BITMAP, &GM, 0, NULL, &Mat);
	BYTE* ptr = new BYTE[size];
	GetGlyphOutline(hdc, c, GGO_GRAY4_BITMAP, &GM, size, ptr, &Mat);

	// デバイスコンテキストとフォントハンドルの開放
	SelectObject(hdc, oldFont);
	DeleteObject(hFont);
	ReleaseDC(NULL, hdc);

	std::vector<DX::XMVECTOR> vert;
	std::vector<uint32_t> id;

	int offset_x = GM.gmptGlyphOrigin.x;
	int offset_y = TM.tmAscent - GM.gmptGlyphOrigin.y;
	int w = GM.gmBlackBoxX + (4 - (GM.gmBlackBoxX % 4)) % 4;
	int h = GM.gmBlackBoxY;
	int Level = 17;
	int x, y;
	uint32_t i = 0;
	DWORD alpha = 0;
	for (y = offset_y+1; y + 1 < offset_y + h; y++)
		for (x = offset_x+1; x + 1 < offset_x + GM.gmBlackBoxX; x++) {
			auto getAlpha = [&ptr, &offset_x, &offset_y, w, Level](int x, int y) {return (255 * ptr[x - offset_x + w * (y - offset_y)]) / (Level - 1); };
			alpha = getAlpha(x, y);
			if (alpha > 0)
			{
				vert.push_back(DX::XMVectorSet(x, -y, 0, 1));vert.push_back(DX::XMVectorSet(0, 0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				vert.push_back(DX::XMVectorSet(x, -y - 1, 0, 1));vert.push_back(DX::XMVectorSet(0, 0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				vert.push_back(DX::XMVectorSet(x + 1, -y - 1, 0, 1));vert.push_back(DX::XMVectorSet(0, 0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				vert.push_back(DX::XMVectorSet(x + 1, -y, 0, 1));vert.push_back(DX::XMVectorSet(0, 0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
				i += 4;

				vert.push_back(DX::XMVectorSet(x, -y, 1, 1));vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				vert.push_back(DX::XMVectorSet(x, -y - 1, 1, 1));vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				vert.push_back(DX::XMVectorSet(x + 1, -y - 1, 1, 1));vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				vert.push_back(DX::XMVectorSet(x + 1, -y, 1, 1));vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
				i += 4;

				//if (x == 0)
				//{
				//	vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y - 1, 1, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y,- 1, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
				//	i += 4;
				//}

				//if (x == offset_x + GM.gmBlackBoxX)
				//{
				//	vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y - 1, 1, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
				//	i += 4;
				//}

				//if (y == offset_y + h)
				//{
				//	vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x+1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x+1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
				//	i += 4;
				//}

				//if (y == 0)
				//{
				//	vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x + 1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x + 1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
				//	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
				//	i += 4;
				//}
			}
			else//no pixel
			{
				bool put_space = false;

				//□□□
				//■□□
				//■■□
				if ( getAlpha(x-1,y) > 0 && getAlpha(x - 1, y + 1) > 0 && getAlpha(x,y+1) > 0)
				{
					vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
					i += 4;
					vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0,0,1,0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;
					vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0,0,-1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y - 1, 1, 1)); vert.push_back(DX::XMVectorSet(0,0,-1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y - 1, 1, 1)); vert.push_back(DX::XMVectorSet(0,0,-1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;

					put_space = true;
				}
				//□□□
				//□□■
				//□■■
				if (getAlpha(x + 1, y) > 0 && getAlpha(x + 1, y + 1) > 0 && getAlpha(x, y + 1) > 0)
				{
					vert.push_back(DX::XMVectorSet(x+1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(-0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(-0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y - 1, 1, 1)); vert.push_back(DX::XMVectorSet(-0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(-0.5f, -0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
					i += 4;
					vert.push_back(DX::XMVectorSet(x, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;
					vert.push_back(DX::XMVectorSet(x, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1,-y-1, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1,-y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;

					put_space = true;
				}

				//■■□
				//■□□
				//□□□
				if (getAlpha(x - 1, y) > 0 && getAlpha(x - 1, y - 1) > 0 && getAlpha(x, y - 1) > 0)
				{
					vert.push_back(DX::XMVectorSet(x, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y - 1, 1, 1)); vert.push_back(DX::XMVectorSet(0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
					i += 4;
					vert.push_back(DX::XMVectorSet(x, -y - 1, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;
					vert.push_back(DX::XMVectorSet(x, -y - 1, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x + 1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;

					put_space = true;
				}

				//□■■
				//□□■
				//□□□
				if (getAlpha(x + 1, y) > 0 && getAlpha(x + 1, y - 1) > 0 && getAlpha(x, y - 1) > 0)
				{
					vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(-0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(-0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(-0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(-0.5f, 0.5f, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
					i += 4;
					vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0,0, 1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;
					vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					vert.push_back(DX::XMVectorSet(x+1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
					id.push_back(i); id.push_back(i + 1); id.push_back(i + 2);
					i += 3;

					put_space = true;
				}

				if (put_space == false)
				{
					//□■□
					//□□□
					//□□□
					if (getAlpha(x, y - 1) > 0)
					{
						vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x+1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x+1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
						i += 4;
					}

					//□□□
					//■□□
					//□□□
					if (getAlpha(x - 1, y) > 0)
					{
						vert.push_back(DX::XMVectorSet(x, -y, 0, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x, -y, 1, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
						i += 4;
					}

					//□□□
					//□□□
					//□■□
					if ( getAlpha(x, y+1) > 0)
					{
						vert.push_back(DX::XMVectorSet(x, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x+1, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x+1, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(0, -1, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
						i += 4;
					}

					//□□□
					//□□■
					//□□□
					if (getAlpha(x + 1, y) > 0)
					{
						vert.push_back(DX::XMVectorSet(x+1, -y, 0, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x+1, -y-1, 0, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x+1, -y-1, 1, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						vert.push_back(DX::XMVectorSet(x+1, -y, 1, 1)); vert.push_back(DX::XMVectorSet(-1, 0, 0, 0)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
						id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
						i += 4;
					}
				}
			}
		}

	for (int i = 0; i < vert.size(); i += 3)
	{
		vert[i] = DX::XMVectorMultiply(vert[i], DX::XMVectorSet(0.02f, 0.02f, 0.02f, 1));
		vert[i] = DX::XMVectorAdd(vert[i], DX::XMVectorSet(-1.0f, 1.0f, -0.02f, 1));
	}

	delete[] ptr;

	d3d::StaticMesh mesh;
	mesh.vbo.Create(&(vert[0]), 12 * sizeof(float),vert.size() * sizeof(DX::XMVECTOR));
	mesh.ibo.Create(&(id[0]), id.size() * sizeof(uint32_t));
	return mesh;
}

}