#pragma pack(push, 1)
typedef struct {
    char code;
    char len;
    int data;
} option_1;

typedef struct {
    char code;
    char len;
    int data;
} option_3;

typedef struct {
    char code;
    char len;
    int data;
} option_50;

typedef struct {
    char code;
    char len;
    unsigned int data;
} option_51;

typedef struct {
    char code;
    char len;
    char data;
} option_53;

typedef struct {
    char code;
    char len;
    int data;
} option_54;

typedef struct {
    char code;
    char len;
    int data;
} option_55;

typedef struct {
    char code;
    char len;
    char data[7];
} option_61;

typedef struct {
	int cookie;
    option_53 option_53;
    option_50 option_50;
    option_54 option_54;
    option_55 option_55;
    char option_255;
} options_Request;

typedef struct {
	int cookie;
    option_53 option_53;
    option_61 option_61;
    option_50 option_50;
    option_55 option_55;
    char option_255;
} options_Discover;

typedef struct {
    void * option_53;
    void * option_1;
    void * option_3;
    void * option_51;
    void * option_54;
} options_Offer;

typedef struct {
    void * option_53;
    void * option_1;
    void * option_3;
    void * option_51;
    void * option_54;
} options_Ack;
#pragma pack(pop)
