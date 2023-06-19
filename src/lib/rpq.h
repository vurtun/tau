#define RPQ_RECT_PACK_VERSION  1

#ifdef RPQ_STATIC
#define RPQ_DEF static
#else
#define RPQ_DEF extern
#endif

typedef struct rpq_context rpq_context;
typedef struct rpq_node    rpq_node;
typedef struct rpq_rect    rpq_rect;

typedef int            rpq_coord;

#define RPQ__MAXVAL  0x7fffffff
// Mostly for internal use, but this is the maximum supported coordinate value.

RPQ_DEF int rpq_pack_rects (rpq_context *context, rpq_rect *rects, int num_rects);
// Assign packed locations to rectangles. The rectangles are of type
// 'rpq_rect' defined below, stored in the array 'rects', and there
// are 'num_rects' many of them.
//
// Rectangles which are successfully packed have the 'was_packed' flag
// set to a non-zero value and 'x' and 'y' store the minimum location
// on each axis (i.e. bottom-left in cartesian coordinates, top-left
// if you imagine y increasing downwards). Rectangles which do not fit
// have the 'was_packed' flag set to 0.
//
// You should not try to access the 'rects' array from another thread
// while this function is running, as the function temporarily reorders
// the array while it executes.
//
// To pack into another rectangle, you need to call rpq_init_target
// again. To continue packing into the same rectangle, you can call
// this function again. Calling this multiple times with multiple rect
// arrays will probably produce worse packing results than calling it
// a single time with the full rectangle array, but the option is
// available.
//
// The function returns 1 if all of the rectangles were successfully
// packed and 0 otherwise.

struct rpq_rect
{
   // reserved for your use:
   int            id;

   // input:
   rpq_coord    w, h;

   // output:
   rpq_coord    x, y;
   int            was_packed;  // non-zero if valid packing

}; // 16 bytes, nominally


RPQ_DEF void rpq_init_target (rpq_context *context, int width, int height, rpq_node *nodes, int num_nodes);
// Initialize a rectangle packer to:
//    pack a rectangle that is 'width' by 'height' in dimensions
//    using temporary storage provided by the array 'nodes', which is 'num_nodes' long
//
// You must call this function every time you start packing into a new target.
//
// There is no "shutdown" function. The 'nodes' memory must stay valid for
// the following rpq_pack_rects() call (or calls), but can be freed after
// the call (or calls) finish.
//
// Note: to guarantee best results, either:
//       1. make sure 'num_nodes' >= 'width'
//   or  2. call rpq_allow_out_of_mem() defined below with 'allow_out_of_mem = 1'
//
// If you don't do either of the above things, widths will be quantized to multiples
// of small integers to guarantee the algorithm doesn't run out of temporary storage.
//
// If you do #2, then the non-quantized algorithm will be used, but the algorithm
// may run out of temporary storage and be unable to pack some rectangles.

RPQ_DEF void rpq_setup_allow_out_of_mem (rpq_context *context, int allow_out_of_mem);
// Optionally call this function after init but before doing any packing to
// change the handling of the out-of-temp-memory scenario, described above.
// If you call init again, this will be reset to the default (false).


RPQ_DEF void rpq_setup_heuristic (rpq_context *context, int heuristic);
// Optionally select which packing heuristic the library should use. Different
// heuristics will produce better/worse results for different data sets.
// If you call init again, this will be reset to the default.

enum
{
   RPQ_HEURISTIC_Skyline_default=0,
   RPQ_HEURISTIC_Skyline_BL_sortHeight = RPQ_HEURISTIC_Skyline_default,
   RPQ_HEURISTIC_Skyline_BF_sortHeight
};


//////////////////////////////////////////////////////////////////////////////
//
// the details of the following structures don't matter to you, but they must
// be visible so you can handle the memory allocations for them

struct rpq_node
{
   rpq_coord  x,y;
   rpq_node  *next;
};

struct rpq_context
{
   int width;
   int height;
   int align;
   int init_mode;
   int heuristic;
   int num_nodes;
   rpq_node *active_head;
   rpq_node *free_head;
   rpq_node extra[2]; // we allocate two extra nodes so optimal user-node-count is 'width' not 'width+2'
};

