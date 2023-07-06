#include <SDL.h>
#include "scene.hpp"
#include "scene_io.hpp"
#include "uimage.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

static bool running = true;
static bool displayed = false;

// idiom for overloaded lambdas - from https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  SDL_Surface *screen;
  scene *s;
  std::shared_ptr< buffer_pair< ucolor > > buf;
};

frame_context *global_context;

// used as emscripten main loop
void render_and_display( void *arg )
{
    frame_context *context;
    context = (frame_context *)arg;
    if( !running && displayed ) {
        global_context->s->ui.mouse_click = false;
        return;
    }
    // unpack context
    SDL_Surface *screen = context->screen;
    uimage& img = (uimage &)(context->buf->get_image());
    vec2i dim = img.get_dim();
    scene &s = *(context->s);
    
    if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

    // copy Lux buffer into SDL buffer
    unsigned int* pixel_ptr = (unsigned int*)screen->pixels;
    unsigned int* base_ptr = img.get_base();
    for( int i=0; i< dim.x * dim.y; i++ ) {
        *pixel_ptr = *base_ptr;
        pixel_ptr++; base_ptr++;
    }

    SDL_Flip(screen);
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

    if( running || !displayed ) {
        s.render();
    }
    global_context->s->ui.mouse_click = false;
    displayed = true;
}

void run_pause() {
    running = !running;
}

void restart() {
    //global_context->frame = 0;
    global_context->s->restart();
    displayed = false;
}

void mouse_move( float x, float y ) {
    global_context->s->ui.mouse_pixel = vec2i( { x, y } );
} 

void mouse_down( bool down ) {
    global_context->s->ui.mouse_down = down;
}

void mouse_over( bool over ) {
    global_context->s->ui.mouse_over = over;
    if( !over ) global_context->s->ui.mouse_down = false;
}

void mouse_click( bool click ) {
    global_context->s->ui.mouse_click = click;
}

// slider value set to range [0.0, 1.0]
void slider_value( std::string value ) {
    //std::cout << "slider value: " << value << std::endl;
    global_context->s->ui.slider_value = ( float )std::stoi( value ) / 1000.0f;
}

int main(int argc, char** argv) {
    vec2i dim( { 512, 512 } );
    //auto dims = img.get_dim();
    emscripten_run_script("console.log('preparing to load scene');");
    //scene s( "diffuser_files/diffuser_brush.json" ); 
    scene s( "nebula_files/nebula_brush.json" ); 
    //scene s( "moon_files/galaxy_moon.json" );
    emscripten_run_script("console.log('scene loaded');");

    std::shared_ptr< buffer_pair< ucolor > > buf( new buffer_pair< ucolor >( dim ) );
    any_buffer_pair_ptr any_buf = buf;
    s.set_output_buffer( any_buf );
    s.ui.canvas_bounds = bb2i( dim );
    SDL_Init(SDL_INIT_VIDEO); 
    SDL_Surface *screen = SDL_SetVideoMode( dim.x, dim.y, 32, SDL_SWSURFACE );

    // pack context
    frame_context context;
    context.screen = screen;
    context.s = &s;
    context.buf = buf;
    global_context = &context;

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif
  emscripten_set_main_loop_arg( render_and_display, &context, -1, 1 );

  SDL_Quit();

  return 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function( "run_pause",    &run_pause );
    function( "restart",      &restart );
    function( "slider_value", &slider_value );
    function( "mouse_move",   &mouse_move );
    function( "mouse_down",   &mouse_down );
    function( "mouse_over",   &mouse_over );
    function( "mouse_click",  &mouse_click );
}