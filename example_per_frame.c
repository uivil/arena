#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Example: Per-Frame Allocation Pattern (Game Loop)
// Demonstrates arena usage in a game-like environment where
// allocations have frame-based lifetimes

typedef struct {
    float x, y;
} Vector2;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
} Particle;

typedef struct {
    Particle *particles;
    int count;
} ParticleSystem;

typedef struct {
    char *text;
    Vector2 position;
    float lifetime;
} DebugText;

typedef struct {
    DebugText *texts;
    int count;
} DebugOverlay;

typedef struct {
    Vector2 start;
    Vector2 end;
} DebugLine;

typedef struct {
    DebugLine *lines;
    int count;
} DebugLines;

// Per-frame data that gets allocated and freed each frame
typedef struct {
    ParticleSystem active_particles;
    DebugOverlay debug_overlay;
    DebugLines debug_lines;
    float delta_time;
} FrameData;

// Permanent game state
typedef struct {
    Arena *persistent_arena;  // Lives for entire game
    Arena frame_arena;        // Reset every frame
    int frame_number;
    Vector2 player_position;
} GameState;

// Simulate particle spawning (uses frame arena)
ParticleSystem spawn_particles(Arena *frame_arena, Vector2 origin, int count) {
    ParticleSystem system;
    system.particles = arena_alloc(frame_arena, sizeof(Particle) * count);
    system.count = count;

    for (int i = 0; i < count; i++) {
        float angle = (float)i / count * 2.0f * 3.14159f;
        system.particles[i].position = origin;
        system.particles[i].velocity.x = cosf(angle) * 2.0f;
        system.particles[i].velocity.y = sinf(angle) * 2.0f;
        system.particles[i].radius = 5.0f;
    }

    return system;
}

// Add debug text (uses frame arena)
void add_debug_text(DebugOverlay *overlay, Arena *frame_arena,
                   const char *format, Vector2 pos) {
    // Allocate space for new text
    int new_count = overlay->count + 1;
    DebugText *new_texts = arena_alloc(frame_arena, sizeof(DebugText) * new_count);

    // Copy existing
    if (overlay->count > 0) {
        memcpy(new_texts, overlay->texts, sizeof(DebugText) * overlay->count);
    }

    // Add new
    new_texts[overlay->count].text = arena_alloc(frame_arena, 256);
    strcpy(new_texts[overlay->count].text, format);
    new_texts[overlay->count].position = pos;
    new_texts[overlay->count].lifetime = 1.0f;

    overlay->texts = new_texts;
    overlay->count = new_count;
}

// Add debug line (uses frame arena)
void add_debug_line(DebugLines *lines, Arena *frame_arena,
                   Vector2 start, Vector2 end) {
    int new_count = lines->count + 1;
    DebugLine *new_lines = arena_alloc(frame_arena, sizeof(DebugLine) * new_count);

    if (lines->count > 0) {
        memcpy(new_lines, lines->lines, sizeof(DebugLine) * lines->count);
    }

    new_lines[lines->count].start = start;
    new_lines[lines->count].end = end;

    lines->lines = new_lines;
    lines->count = new_count;
}

// Process a single frame
void process_frame(GameState *state, float delta_time) {
    // Reset frame arena - all allocations from last frame are freed!
    arena_reset(&state->frame_arena);

    // Allocate frame data on frame arena
    FrameData *frame = arena_alloc(&state->frame_arena, sizeof(FrameData));
    memset(frame, 0, sizeof(FrameData));
    frame->delta_time = delta_time;

    // Update player
    state->player_position.x += sinf(state->frame_number * 0.1f) * 0.5f;
    state->player_position.y += cosf(state->frame_number * 0.1f) * 0.5f;

    // Spawn particles (allocated on frame arena)
    if (state->frame_number % 30 == 0) {
        frame->active_particles = spawn_particles(&state->frame_arena,
                                                  state->player_position, 8);
    }

    // Add debug text (allocated on frame arena)
    Vector2 text_pos = {10, 10};
    char buffer[256];
    sprintf(buffer, "Frame: %d", state->frame_number);
    add_debug_text(&frame->debug_overlay, &state->frame_arena, buffer, text_pos);

    text_pos.y += 20;
    sprintf(buffer, "Player: (%.1f, %.1f)", state->player_position.x,
            state->player_position.y);
    add_debug_text(&frame->debug_overlay, &state->frame_arena, buffer, text_pos);

    text_pos.y += 20;
    sprintf(buffer, "Frame arena: %zu bytes", state->frame_arena.offset);
    add_debug_text(&frame->debug_overlay, &state->frame_arena, buffer, text_pos);

    // Add debug lines
    Vector2 line_start = {0, 0};
    Vector2 line_end = state->player_position;
    add_debug_line(&frame->debug_lines, &state->frame_arena, line_start, line_end);

    // Render (in a real game, this would draw to screen)
    printf("Frame %d:\n", state->frame_number);
    printf("  Player at (%.1f, %.1f)\n",
           state->player_position.x, state->player_position.y);

    if (frame->active_particles.count > 0) {
        printf("  Particles: %d\n", frame->active_particles.count);
    }

    printf("  Debug texts: %d\n", frame->debug_overlay.count);
    for (int i = 0; i < frame->debug_overlay.count; i++) {
        printf("    '%s'\n", frame->debug_overlay.texts[i].text);
    }

    printf("  Debug lines: %d\n", frame->debug_lines.count);
    printf("  Frame arena used: %zu bytes\n", state->frame_arena.offset);
    printf("\n");

    state->frame_number++;

    // When this function returns, all frame allocations are still valid
    // until arena_reset() at the start of the next frame
}

void example_game_loop(void) {
    printf("=== Example: Per-Frame Allocation (Game Loop) ===\n\n");

    // Initialize game state
    GameState state = {0};
    state.persistent_arena = malloc(sizeof(Arena));
    *state.persistent_arena = arena_create(1024 * 1024); // 1MB persistent
    state.frame_arena = arena_create(256 * 1024);        // 256KB per frame
    state.frame_number = 0;
    state.player_position = (Vector2){100, 100};

    // Run game loop for 5 frames
    for (int i = 0; i < 5; i++) {
        float delta_time = 0.016f; // ~60 FPS
        process_frame(&state, delta_time);
    }

    // Cleanup
    arena_destroy(state.persistent_arena);
    free(state.persistent_arena);
    arena_destroy(&state.frame_arena);

    printf("Game loop complete!\n\n");
}

// Example: Per-Request Allocation (Server Pattern)
// Demonstrates arena usage in a server where allocations
// have request-based lifetimes

typedef struct {
    char *method;
    char *path;
    char **headers;
    int header_count;
    char *body;
} HttpRequest;

typedef struct {
    int status_code;
    char **headers;
    int header_count;
    char *body;
} HttpResponse;

// Parse HTTP request (uses request arena)
HttpRequest parse_request(const char *raw, Arena *request_arena) {
    ArenaTemp scratch = scratch_begin((Arena*[]){request_arena}, 1);

    HttpRequest req = {0};

    // Parse first line (simplified)
    const char *space1 = strchr(raw, ' ');
    const char *space2 = strchr(space1 + 1, ' ');

    size_t method_len = space1 - raw;
    req.method = arena_alloc(request_arena, method_len + 1);
    memcpy(req.method, raw, method_len);
    req.method[method_len] = '\0';

    size_t path_len = space2 - (space1 + 1);
    req.path = arena_alloc(request_arena, path_len + 1);
    memcpy(req.path, space1 + 1, path_len);
    req.path[path_len] = '\0';

    // Count headers (simplified - just add a few)
    req.header_count = 2;
    req.headers = arena_alloc(request_arena, sizeof(char*) * req.header_count);
    req.headers[0] = arena_alloc(request_arena, 50);
    strcpy(req.headers[0], "Host: example.com");
    req.headers[1] = arena_alloc(request_arena, 50);
    strcpy(req.headers[1], "User-Agent: ArenaClient/1.0");

    scratch_end(scratch);
    return req;
}

// Handle request and generate response
HttpResponse handle_request(HttpRequest req, Arena *response_arena) {
    ArenaTemp scratch = scratch_begin((Arena*[]){response_arena}, 1);

    HttpResponse resp = {0};
    resp.status_code = 200;

    // Generate response body based on path
    if (strcmp(req.path, "/") == 0) {
        resp.body = arena_alloc(response_arena, 256);
        sprintf(resp.body, "Welcome! Method: %s", req.method);
    }
    else if (strcmp(req.path, "/api/data") == 0) {
        // Simulate some data processing using scratch
        char *temp_buffer = arena_alloc(scratch.arena, 1024);
        sprintf(temp_buffer, "{\"data\":[1,2,3,4,5],\"method\":\"%s\"}", req.method);

        // Copy to response
        size_t len = strlen(temp_buffer) + 1;
        resp.body = arena_alloc(response_arena, len);
        strcpy(resp.body, temp_buffer);
    }
    else {
        resp.status_code = 404;
        resp.body = arena_alloc(response_arena, 50);
        strcpy(resp.body, "Not Found");
    }

    // Add response headers
    resp.header_count = 1;
    resp.headers = arena_alloc(response_arena, sizeof(char*));
    resp.headers[0] = arena_alloc(response_arena, 100);
    sprintf(resp.headers[0], "Content-Length: %zu", strlen(resp.body));

    scratch_end(scratch);
    return resp;
}

void handle_http_request(const char *raw_request) {
    // Create request arena - lives for duration of request processing
    Arena request_arena = arena_create(64 * 1024);  // 64KB per request

    // Parse request
    HttpRequest req = parse_request(raw_request, &request_arena);

    printf("Request: %s %s\n", req.method, req.path);
    printf("  Headers:\n");
    for (int i = 0; i < req.header_count; i++) {
        printf("    %s\n", req.headers[i]);
    }

    // Handle request (uses same arena for response)
    HttpResponse resp = handle_request(req, &request_arena);

    printf("Response: %d\n", resp.status_code);
    printf("  Headers:\n");
    for (int i = 0; i < resp.header_count; i++) {
        printf("    %s\n", resp.headers[i]);
    }
    printf("  Body: %s\n", resp.body);
    printf("  Request arena used: %zu bytes\n", request_arena.offset);
    printf("\n");

    // Destroy arena - frees all request and response data at once
    arena_destroy(&request_arena);
}

void example_server_requests(void) {
    printf("=== Example: Per-Request Allocation (Server) ===\n\n");

    scratch_init(1024 * 1024);

    // Simulate handling multiple requests
    const char *requests[] = {
        "GET / HTTP/1.1",
        "POST /api/data HTTP/1.1",
        "GET /notfound HTTP/1.1",
    };

    for (int i = 0; i < 3; i++) {
        handle_http_request(requests[i]);
    }

    scratch_cleanup();
}

int main(void) {
    example_game_loop();
    example_server_requests();

    return 0;
}
