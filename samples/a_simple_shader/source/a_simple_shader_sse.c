/*
    SSE2 implementation of GLSL shader from http://glslsandbox.com/e#57400.0

    #liborbis port, 2019, masterzorag@gmail.com

    - http://gruntthepeon.free.fr/ssemath/
*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __PS4__
#include <orbis2d.h>  // ATTR_*
extern Orbis2dConfig *orbconf;  // hook main 2d conf to get framebuffer address
#else
#include "pixel.h"         // SDL bindings: hook the SDL_surface from main() with init_pixel_SDL()
#include "orbisXbmFont.h"  // ARGB
#endif


// Clang Extended Vectors
typedef float vec2 __attribute__((ext_vector_type(2)));
typedef float vec3 __attribute__((ext_vector_type(3)));
typedef float vec4 __attribute__((ext_vector_type(4)));


static float Time = 0.f;
/*uniform vec2 mouse; */

//static const vec2 resolution = { ATTR_WIDTH, ATTR_HEIGHT };
//static       vec4 gl_FragColor;
static const vec2 __attribute__((aligned(16))) step = { 1. /ATTR_WIDTH, 1. /ATTR_HEIGHT };

#include <immintrin.h>    // oh, yes!
#include "sse_mathfun.h"  // sin_ps()


#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif


/* http://glslsandbox.com/e#57400.0 */

#if 0  /* original shader */
    vec2   p = ( gl_FragCoord.xy / resolution.xy ) + 0.5;
    float sx = (amp)*1.9 * sin( 4.0 * (freq) * (p.x-phase) - 6.0 * (speed)*Time);
    float dy = 43./ ( 60. * abs(4.9*p.y - sx - 1.2));
    //dy    +=  1./ ( 60. * length(p - vec2(p.x, 0.)));
  
    gl_FragColor = vec4( (p.x + 0.05) * dy, 0.2 * dy, dy, 2.0 );
#endif

#define speed 0.2
#define freq  0.8
#define amp   0.9
#define phase 2.5

/* SSE2 implementation */
void glsl_e57400_sse( int variant )
{
    //Time = 0.f;
    register uint32_t *pixel = (uint32_t *)orbconf->surfaceAddr[orbconf->currentBuffer];

    register vec2 gl_FragCoord = { 0, 1 };  // draw from upperleft
    //const    vec2 step         = (vec2)( 1. /resolution );
    
    register int __attribute__((aligned(16))) w, h;
    __m128 __attribute__((aligned(16))) px, t1, t2;
    __m128 __attribute__((aligned(16))) _speed, _freq, _amp, _phase;
    __m128 __attribute__((aligned(16))) _v1, _v2, _v3, _v4, _v5, _v6, _v7;

    switch( variant )
    {
      case 2: /* http://glslsandbox.com/e#57659.0 */
        _speed = _mm_set1_ps( 0.3 ),    _freq  = _mm_set1_ps(  0.8 );
        _amp   = _mm_set1_ps( 0.9 ),    _phase = _mm_set1_ps(  2.5 );
        _v1    = _mm_set1_ps( 1.2 ),    _v2    = _mm_set1_ps(  5.0 );
        _v3    = _mm_set1_ps( 6.0 ),    _v4    = _mm_set1_ps( 43.0 );
        _v5    = _mm_set1_ps( 0.025 ),  _v6    = _mm_set1_ps( 90.0 );
        _v7    = _mm_set1_ps( 0.05 );   break;
      case 1: /* http://glslsandbox.com/e#57444.0 */
        _speed = _mm_set1_ps( 0.2 ),    _freq  = _mm_set1_ps(   0.7 );
        _amp   = _mm_set1_ps( 0.5 ),    _phase = _mm_set1_ps(   0.5 );
        _v1    = _mm_set1_ps( 1.2 ),    _v2    = _mm_set1_ps(   9.0 );
        _v3    = _mm_set1_ps( 3.5 ),    _v4    = _mm_set1_ps( 100.0 );
        _v5    = _mm_set1_ps( 0.05 ),   _v6    = _mm_set1_ps(  90.0 );
        _v7    = _mm_set1_ps( 0.2 );    break;
      case 0: /* http://glslsandbox.com/e#57400.0 */
        _speed = _mm_set1_ps( speed ),  _freq  = _mm_set1_ps( freq );
        _amp   = _mm_set1_ps( amp ),    _phase = _mm_set1_ps( phase );
        _v1    = _mm_set1_ps( 1.9 ),    _v2    = _mm_set1_ps(  4.0 );
        _v3    = _mm_set1_ps( 6.0 ),    _v4    = _mm_set1_ps( 43.0 );
        _v5    = _mm_set1_ps( 0.5 ),    _v6    = _mm_set1_ps( 60.0 );
        _v7    = _mm_set1_ps( 0.2 );    break;
    }

    #pragma clang loop unroll(disable)
    for(h=0; h < ATTR_HEIGHT; h++) // each row in the framebuffer
    {
        #pragma clang loop unroll_count(4)
        for(w=0; w < ATTR_WIDTH /4; w++) // each horizontal 4 pixels in (h) row
        {
         /* px = gl_FragCoord.x - 0.5; but for (x4) pixels */
         /* px = _mm_set_ps(gl_FragCoord.x - 0.5 + step.x *3, gl_FragCoord.x - 0.5 + step.x *2,
                            gl_FragCoord.x - 0.5 + step.x *1, gl_FragCoord.x - 0.5 + step.x *0);
            // load in reverse order*/
            t1 = _mm_mul_ps( _mm_set_ps( 3., 2., 1., 0. ),
                             _mm_set1_ps( step.x ) );               // t1 = incremental step.x
            px = _mm_add_ps( t1,
                             _mm_set1_ps( gl_FragCoord.x - 0.5) );  // t2 = px (x4)

         /* sx = (amp)*1.9 * sin( 4.0 * (freq) * (p.x-phase) - 6.0 * (speed)*Time); */

            t1 = _mm_sub_ps(px, _phase);    // t1 = (p.x-phase)
            t2 = _mm_mul_ps(_v2, _freq);    // t2 = 4.0 * freq
            t2 = _mm_mul_ps(t2, t1);        // t2 = 4.0 * freq * (p.x-phase)

            t1 = _mm_set1_ps( Time );
            t1 = _mm_mul_ps(t1, _v3);
            t1 = _mm_mul_ps(t1, _speed);
            t1 = _mm_sub_ps(t2, t1);        // t1 = 4.0 * freq * (p.x-phase) - 6.0 * (speed)*Time

            t2 = sin_ps(t1);

            t1 = _mm_mul_ps(_v1, _amp );
            t2 = _mm_mul_ps(t1, t2);        // t2 = (amp)*1.9 * sin()

            /*if(w==2 && h==10) {
              // Display the elements of the result vector 
                //1.
                    float* f = (float*)&t2;
                    printf("%.3f %.3f %.3f %.3f\n", f[0], f[1], f[2], f[3]);  // sx
                //2.
                    memcpy(&B, &t2, sizeof(__m128));
                    printf("sx: %.3f, %.3f, %.3f, %.3f\n", B[3], B[2], B[1], B[0]);  // sx
                //3.
                    _mm_storer_ps(color, t2); // in reverse order
                    printf("sx: %.3f, %.3f, %.3f, %.3f\n", color[0], color[1], color[2], color[3]);  // sx
            }*/

         /* dy = 43./ ( 60. * fabs(4.9*p.y - sx - 1.2)); */
 
            t1 = _mm_set1_ps( 4.9*(gl_FragCoord.y - 0.25) ),
            t1 = _mm_sub_ps(t1, t2);                            // t1 = 4.9*p.y - sx
            t2 = _mm_set1_ps( 1.2 ),  t1 = _mm_sub_ps(t1, t2);  // 4.9*p.y - sx - 1.2

            /* "The maximum of -x and x should be abs(x)." */
            t2 = _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), t1), t1); // fabs( 4.9*p.y - sx - 1.2 )

            t2 = _mm_mul_ps(_v6, t2);  // 60. * fabs(4.9*p.y - sx - 1.2));
            t2 = _mm_div_ps(_v4, t2);  // 43./ ( 60. * fabs(4.9*p.y - sx - 1.2))

         /* gl_FragColor = (vec4){ (p.x + 0.5) * dy, 0.2 * dy, dy, 2.0 }; */

            /* sanitize vectors for RGB, this is not GLSL!
               clamp floats to 0.-1. then
               mul(256) to get 0-255 channels color, as u8 */
            
            /* (p.x + 0.5) * dy */
            px = _mm_add_ps(px, _v5);
            px = _mm_mul_ps(px, t2);   // * dy
            /* clamp 0.f-1.f */
            px = _mm_min_ps( _mm_max_ps(px, _mm_set1_ps(0.)), _mm_set1_ps(1.f) ); // x4 R
            px = _mm_mul_ps(px, _mm_set1_ps( 0xFF ));

            // _mm_storer_ps(B, px);  // in reverse order
            //memcpy(&B, &px, sizeof(__m128));

            /* 0.2 * dy, dy */
            t1 = _mm_mul_ps(_v7, t2);   // * dy
            /* clamp 0.f-1.f */
            t1 = _mm_min_ps( _mm_max_ps(t1, _mm_set1_ps( 0. )), _mm_set1_ps( 1.f ) ); // x4 G
            t1 = _mm_mul_ps(t1, _mm_set1_ps( 0xFF ));

            // _mm_storer_ps(G, t1),                 // in reverse order
            //memcpy(&G, &t1, sizeof(__m128));

            /* clamp 0.f-1.f */
            t2 = _mm_min_ps( _mm_max_ps(t2, _mm_set1_ps( 0. )), _mm_set1_ps( 1.f ) ); // x4 B
            t2 = _mm_mul_ps(t2, _mm_set1_ps( 0xFF ));

            // _mm_storer_ps(R, t2);                 // in reverse order
            //memcpy(&R, &t2, sizeof(__m128));

            /* unpack, compose pixel (x4) colors */
            /*float* r = (float*)&t2;
            float* g = (float*)&t1;
            float* b = (float*)&px;*/
            #pragma clang loop unroll_count(4)
            for(int i=0; i<4; i++)
            {
                 //gl_FragColor = (vec4){ B[i], G[i], R[i], 2.0 };
                 /*printf("color = 0x%.8X\n", ARGB(0xFF, (unsigned char)(0xFF * gl_FragColor.r),
                                                         (unsigned char)(0xFF * gl_FragColor.g),
                                                         (unsigned char)(0xFF * gl_FragColor.b)));*/
                 /* paint pixel */
                 /*orbis2dDrawPixelColor(w *4 +i, h, ARGB(0xFF, (unsigned char)(gl_FragColor.r),
                                                              (unsigned char)(gl_FragColor.g),
                                                              (unsigned char)(gl_FragColor.b)));*/
                 /*orbis2dDrawPixelColor(w *4 +i, h, ARGB(0xFF, (unsigned char)(b[i]),
                                                              (unsigned char)(g[i]),
                                                              (unsigned char)(r[i])));*/
                 *pixel++ = ARGB(0xFF, (unsigned char)(px[i]),
                                       (unsigned char)(t1[i]),
                                       (unsigned char)(t2[i]));  // ABGR
            }
            gl_FragCoord.x += step.x *4; // advance X *4
        }
        gl_FragCoord.x  = 0.,
        gl_FragCoord.y -= step.y; // advance Y, next row
        //Time += 0.0001976 /8; // animate (blur?)
    }
    Time += 0.01976 *2 ; // animate
    //printf("color = 0x%.8X\n", color);  //color = FF370B37
}
