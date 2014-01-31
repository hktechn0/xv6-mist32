#define GCI_OFFSET 0x200

#define GCI_NODE_MAX 4
#define GCI_KMC_NUM 0
#define GCI_DISPLAY_NUM 1
#define GCI_MMCC_NUM 2

/* Area Size */
#define GCI_HUB_SIZE 0x400
#define GCI_HUB_HEADER_SIZE 0x100
#define GCI_NODE_SIZE 0x400

/* KMC */
#define KMC_SCANCODE_VALID 0x100
#define KMC_BREAKCODE 0xf0

/* Display */
#define DISPLAY_CHAR_WIDTH 80
#define DISPLAY_CHAR_HEIGHT 34
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480
#define DISPLAY_BITMAP_OFFSET 0xc000

/* MMCC */
#define MMCC_INITIAL_COMMAND 0x5a5a /* dummy */
#define MMCC_BUFFER_OFFSET 0x040
#define MMCC_BUFFER_SIZE 0x200
#define MMCC_SECTOR_SIZE 512

/* GCI struct */
struct gci_hub_info {
  unsigned int total;
  unsigned int space_size;
};

struct gci_hub_node {
  unsigned int size;
  unsigned int priority;
  unsigned int _reserved1;
  unsigned int _reserved2;
  unsigned int _reserved3;
  unsigned int _reserved4;
  unsigned int _reserved5;
  unsigned int _reserved6;
};

struct gci_node_info {
  unsigned int area_size;
  unsigned int int_priority;
  volatile unsigned int int_factor;
  unsigned int _reserved;
};

struct gci_node {
  struct gci_node_info *node_info;
  void *device_area;
};

/* MMCC */
struct gci_mmcc {
  volatile unsigned int init_command;
  volatile unsigned int sector_read;
  volatile unsigned int sector_write;
};

/* GCI Global */
extern struct gci_hub_info *gci_hub;
extern struct gci_hub_node *gci_hub_nodes;
extern struct gci_node gci_nodes[4];

/* inline functions */
static inline unsigned int gci_interrupt_ack(struct gci_node *node)
{
  volatile unsigned int factor;

  factor = node->node_info->int_factor;
  return factor;
}

static inline unsigned int gci_kmc_scancode(struct gci_node *kmc)
{
  return *(volatile unsigned int *)kmc->device_area;
}
