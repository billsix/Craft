#ifndef _main_h_
#define _main_h_

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define WORKERS 4
#define MAX_TEXT_LENGTH 256
#define MAX_NAME_LENGTH 32
#define MAX_PATH_LENGTH 256
#define MAX_ADDR_LENGTH 256

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

#define MODE_OFFLINE 0
#define MODE_ONLINE 1

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

typedef struct {
  Map map;
  Map lights;
  SignList signs;
  int p;
  int q;
  int faces;
  int sign_faces;
  int dirty;
  int miny;
  int maxy;
  uint32_t buffer;
  uint32_t sign_buffer;
} Chunk;

typedef struct {
  int p;
  int q;
  int load;
  Map *block_maps[3][3];
  Map *light_maps[3][3];
  int miny;
  int maxy;
  int faces;
  float *data;
} WorkerItem;

typedef struct {
  int index;
  int state;
  thrd_t thrd;
  mtx_t mtx;
  cnd_t cnd;
  WorkerItem item;
} Worker;

typedef struct {
  int x;
  int y;
  int z;
  int w;
} Block;

/* The position of a player, annd it's orientation (which
 * way it's looking, both side to side, and up or down
 */
typedef struct {
  /*
   * Position
   *
   * The x, y, and z components of the position of
   * the moving entity, aka a player
   */
  float x;
  float y;
  float z;
  /*
   * Rotation
   *
   * the amount of radians rotated around the x axis.
   * it follows the right hand rule, so positive theta
   * implies that y rotates towards z, and that z rotates
   * towards -y
   */
  float rx;
  /* the amount of radians rotated around the y axis.
   * it follows the right hand rule, so z rotates towards
   * x, and x rotates towards -z
   *
   * A sequence of rotations is not commutative, so
   * the order in which we apply them will matter.
   *
   * First, the rotation around the y axis will happen first,
   * looking side to side.  Then, the rotation around x will be
   * applied, to look up or down.
   */
  float ry;

  /*
   * TODO : figure out what t is and explain it.
   */
  float t;
} PositionAndOrientation;

typedef struct {
  int id;
  char name[MAX_NAME_LENGTH];
  PositionAndOrientation positionAndOrientation;
  PositionAndOrientation positionAndOrientation1;
  PositionAndOrientation positionAndOrientation2;
  uint32_t buffer;
} Player;

typedef struct {
  GLFWwindow *window;
  Worker workers[WORKERS];
  Chunk chunks[MAX_CHUNKS];
  int chunk_count;
  int create_radius;
  int render_radius;
  int delete_radius;
  int sign_radius;
  Player players[MAX_PLAYERS];
  int player_count;
  int typing;
  char typing_buffer[MAX_TEXT_LENGTH];
  int message_index;
  char messages[MAX_MESSAGES][MAX_TEXT_LENGTH];
  int width;
  int height;
  int observe1;
  int observe2;
  int flying;
  int item_index;
  int scale;
  int ortho;
  float fov;
  int suppress_char;
  int mode;
  int mode_changed;
  char db_path[MAX_PATH_LENGTH];
  char server_addr[MAX_ADDR_LENGTH];
  int server_port;
  int day_length;
  int time_changed;
  Block block0;
  Block block1;
  Block copy0;
  Block copy1;
} Model;

//   and only store onto programs; get rid
//   of the Attrib struct
typedef struct {
  uint32_t program;
  uint32_t position;
  uint32_t normal;
  uint32_t uv;
  uint32_t matrix;
  uint32_t sampler;
  uint32_t camera;
  uint32_t timer;
  uint32_t sky_sampler;
  uint32_t daylight;
  uint32_t fog_distance;
  uint32_t ortho;
} Block_Attributes;

typedef struct {
  uint32_t program;
  uint32_t position;
  uint32_t matrix;
} Line_Attributes;

typedef struct {
  uint32_t program;
  uint32_t position;
  uint32_t uv;
  uint32_t matrix;
  uint32_t sampler;
  uint32_t is_sign;
} Text_Attributes;

typedef struct {
  uint32_t program;
  uint32_t position;
  uint32_t normal;
  uint32_t uv;
  uint32_t matrix;
  uint32_t sampler;
  uint32_t timer;
} Sky_Attributes;

extern Model *g;

float time_of_day();
int _gen_sign_buffer(float *data, float x, float y, float z, int face,
                     const char *text);

#endif