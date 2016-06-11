#include "../common.h"
#include "SDL_screen.h"

#ifdef SDL_OPENGL_BLIT
  #include "SDL_opengl.h"
#endif

SDL_Surface *screen = NULL;
SDL_Surface *real_screen = NULL;
u32 last_scale_factor;
u8 *real_screen_pixels;

#define plot_pixel_2x(offset)                                                 \
  dest_pixels[offset] = current_pixel;                                        \
  dest_pixels[offset + 1] = current_pixel                                     \

#define plot_pixel_3x(offset)                                                 \
  plot_pixel_2x(offset);                                                      \
  dest_pixels[offset + 2] = current_pixel                                     \

#define plot_pixel_4x(offset)                                                 \
  plot_pixel_3x(offset);                                                      \
  dest_pixels[offset + 3] = current_pixel                                     \

void copy_screen_scale(SDL_Surface *dest, SDL_Surface *src, u32 scale_factor)
{
  u32 src_pitch = src->pitch / 2;
  u16 *src_pixels = src->pixels;
  u32 dest_pitch = dest->pitch / 2;
  u16 *dest_pixels = dest->pixels;
  u32 src_skip = src_pitch - src->w;
  u32 dest_skip;
  u32 src_width = src->w;
  u32 src_height = src->h;

  u32 current_pixel;
  u32 i, i2;

  if((src->w * scale_factor) < dest->w)
    dest_pixels += (dest->w - (src->w * scale_factor)) / 2;

  if((src->h * scale_factor) < dest->h)
    dest_pixels += ((dest->h - (src->w * scale_factor)) / 2) * dest_pitch;

  switch(scale_factor)
  {
    case 2:
      dest_skip = (dest_pitch - (src_width * 2)) + dest_pitch;

      for(i = 0; i < src_height; i++)
      {
        for(i2 = 0; i2 < src_width; i2++)
        {
          current_pixel = *src_pixels;
          plot_pixel_2x(0);
          plot_pixel_2x(dest_pitch);

          src_pixels++;
          dest_pixels += 2;
        }
        src_pixels += src_skip;
        dest_pixels += dest_skip;
      }
      break;

    case 3:
    {
      u32 dest_pitch2 = dest_pitch * 2;

      dest_skip = (dest_pitch - (src_width * 3)) + dest_pitch2;

      for(i = 0; i < src_height; i++)
      {
        for(i2 = 0; i2 < src_width; i2++)
        {
          current_pixel = *src_pixels;
          plot_pixel_3x(0);
          plot_pixel_3x(dest_pitch);
          plot_pixel_3x(dest_pitch2);

          src_pixels++;
          dest_pixels += 3;
        }
        src_pixels += src_skip;
        dest_pixels += dest_skip;
      }
      break;
    }


    case 4:
    {
      u32 dest_pitch2 = dest_pitch * 2;
      u32 dest_pitch3 = dest_pitch * 3;

      dest_skip = (dest_pitch - (src_width * 4)) + dest_pitch3;

      for(i = 0; i < src_height; i++)
      {
        for(i2 = 0; i2 < src_width; i2++)
        {
          current_pixel = *src_pixels;
          plot_pixel_4x(0);
          plot_pixel_4x(dest_pitch);
          plot_pixel_4x(dest_pitch2);
          plot_pixel_4x(dest_pitch3);

          src_pixels++;
          dest_pixels += 4;
        }
        src_pixels += src_skip;
        dest_pixels += dest_skip;
      }
      break;
    }
  }
}

#ifdef SDL_OPENGL_BLIT

GLuint pixel_buffer_objects[2];
GLuint texture_handle;
GLubyte pbo_buffer[320 * 240 * 2];
GLubyte *pbo_pixels;
u32 pbo_index = 0;
u32 pbo_next_index = 1;

void bind_texture(void)
{
  glBindTexture(GL_TEXTURE_2D, texture_handle);
  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pixel_buffer_objects[pbo_index]);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 320, 240, GL_RGBA,
   GL_UNSIGNED_SHORT_1_5_5_5_REV, 0);

  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
    pixel_buffer_objects[pbo_next_index]);
  glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 320 * 240 * 2, 0,
   GL_STREAM_DRAW_ARB);
  pbo_pixels =
   (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
}

#endif

void update_screen()
{
  if(last_scale_factor != config.scale_factor)
    set_screen_resolution(320, 240);

#ifdef SDL_OPENGL_BLIT
  if(config.use_opengl)
  {
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    glBindTexture(GL_TEXTURE_2D, texture_handle);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3f(0, 0, 0);
    glTexCoord2f(1, 0);
    glVertex3f(1, 0, 0);
    glTexCoord2f(1, 1);
    glVertex3f(1, 1, 0);
    glTexCoord2f(0, 1);
    glVertex3f(0, 1, 0);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    SDL_GL_SwapBuffers();

    pbo_index = (pbo_index + 1) % 2;
    pbo_next_index = (pbo_next_index + 1) % 2;

    bind_texture();
  }
  else
#endif
  {
    if((config.scale_factor == SCALE_1x) ||
     (config.scale_factor == SCALE_FULLSCREEN))
    {
      SDL_Flip(screen);
    }
    else
    {
      copy_screen_scale(real_screen, screen, config.scale_factor);
      SDL_Flip(real_screen);
    }
  }
}

void set_screen_resolution(u32 width, u32 height)
{
#ifdef SDL_OPENGL_BLIT
  if(config.use_opengl)
  {
    screen = SDL_SetVideoMode(width, height, 16, SDL_OPENGL);

    glShadeModel(GL_FLAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); 

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glDisable(GL_COLOR_MATERIAL);

    glClearColor(0, 0, 0, 0);

    glGenTextures(1, &texture_handle);
    glBindTexture(GL_TEXTURE_2D, texture_handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 0,
     GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, (GLvoid *)pbo_buffer);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(2, pixel_buffer_objects);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pixel_buffer_objects[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, 320 * 240 * 2, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pixel_buffer_objects[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, 320 * 240 * 2, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    glOrtho(0, 1, 0, 1, -1, 1);
    bind_texture();
  }
  else
#endif
  {
    u16 *old_pixels = NULL;

    if(screen != NULL)
    {
      old_pixels = malloc(320 * 240 * 2);
      copy_screen(old_pixels);
      SDL_FreeSurface(screen);
    }

    switch(config.scale_factor)
    {
      case SCALE_FULLSCREEN:
        screen = SDL_SetVideoMode(width, height, 16, SDL_FULLSCREEN);
        real_screen_pixels = screen->pixels;

        if(old_pixels != NULL)
          blit_screen(old_pixels);

        break;

      case SCALE_1x:
        screen = SDL_SetVideoMode(width, height, 16, 0);
  
        if(old_pixels != NULL)
          blit_screen(old_pixels);
        break;

      default:
        screen = SDL_CreateRGBSurface(0, width, height, 16, 0, 0, 0, 0);

        if(real_screen != NULL)
          SDL_FreeSurface(real_screen);

        real_screen = SDL_SetVideoMode(width * config.scale_factor,
         height * config.scale_factor, 16, 0);

        if(old_pixels != NULL)
        {
          blit_screen(old_pixels);
          copy_screen_scale(real_screen, screen, config.scale_factor);
        }
        break;
    }

    if(old_pixels != NULL)
      free(old_pixels);

    last_scale_factor = config.scale_factor;
  }

  SDL_WM_SetCaption("Temper PC-Engine Emulator", "Temper");
}

void *get_screen_ptr()
{
#ifdef SDL_OPENGL_BLIT
  if(config.use_opengl)
    return pbo_pixels;
#endif

  return screen->pixels;
}

u32 get_screen_pitch()
{
  if(config.use_opengl)
    return 320;

  return (screen->pitch / 2);
}

void clear_screen()
{
  u32 i;
  u32 pitch = get_screen_pitch();
  u16 *pixels = get_screen_ptr();

  for(i = 0; i < 240; i++)
  {
    memset(pixels, 0, 320 * 2);
    pixels += pitch;
  }
}

void clear_line_edges(u32 line_number, u32 color, u32 edge, u32 middle)
{
  u32 *dest = (u32 *)((u16 *)get_screen_ptr() +
   (line_number * get_screen_pitch()));
  u32 i;

  color |= (color << 16);

  edge /= 2;
  middle /= 2;

  for(i = 0; i < edge; i++)
  {
    *dest = color;
    dest++;
  }

  dest += middle;

  for(i = 0; i < edge; i++)
  {
    *dest = color;
    dest++;
  }
}

void set_single_buffer_mode()
{
}

void set_multi_buffer_mode()
{
}

void clear_all_buffers()
{
}

