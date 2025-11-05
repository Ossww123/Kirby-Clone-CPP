// 미사용

#pragma once
#include <windows.h>
#include <vector>

namespace engine {
    namespace debug {

        struct LineCmd { int x1 , y1 , x2 , y2; COLORREF color; };
        struct RectCmd { int x , y , w , h;     COLORREF color; };

        inline std::vector<LineCmd>& Lines ( ) { static std::vector<LineCmd> v; return v; }
        inline std::vector<RectCmd>& Rects ( ) { static std::vector<RectCmd> v; return v; }

        inline void BeginFrame ( ) {
            Lines ( ).clear ( );
            Rects ( ).clear ( );
        }

        // 스크린 좌표(이미 오프셋 적용됨)
        inline void Line ( int x1 , int y1 , int x2 , int y2 , COLORREF c ) { Lines ( ).push_back ( { x1,y1,x2,y2,c } ); }
        inline void Rect ( int x , int y , int w , int h , COLORREF c ) { Rects ( ).push_back ( { x,y,w,h,c } ); }

        // 월드 좌표 → 스크린 변환 헬퍼
        inline void WorldRect ( int wx , int wy , int w , int h , int ox , int oy , COLORREF c ) {
            Rect ( wx - ox , wy - oy , w , h , c );
        }
        inline void WorldLine ( int x1 , int y1 , int x2 , int y2 , int ox , int oy , COLORREF c ) {
            Line ( x1 - ox , y1 - oy , x2 - ox , y2 - oy , c );
        }

        inline void Flush ( HDC dc ) {
            // 선
            for ( const auto& e : Lines ( ) ) {
                HPEN pen = CreatePen ( PS_SOLID , 1 , e.color );
                HGDIOBJ old = SelectObject ( dc , pen );
                MoveToEx ( dc , e.x1 , e.y1 , nullptr );
                LineTo ( dc , e.x2 , e.y2 );
                SelectObject ( dc , old ); DeleteObject ( pen );
            }
            // 사각형(아웃라인)
            for ( const auto& r : Rects ( ) ) {
                HPEN pen = CreatePen ( PS_SOLID , 1 , r.color );
                HGDIOBJ oldPen = SelectObject ( dc , pen );
                HGDIOBJ oldBr = SelectObject ( dc , GetStockObject ( HOLLOW_BRUSH ) );
                Rectangle ( dc , r.x , r.y , r.x + r.w , r.y + r.h );
                SelectObject ( dc , oldBr ); SelectObject ( dc , oldPen ); DeleteObject ( pen );
            }
        }

    }
} // namespace engine::debug
