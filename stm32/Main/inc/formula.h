#ifndef __FORMULA_H
#define __FORMULA_H
#include "obd.h"
typedef enum {OFF = 0, ON} SwitchType;
typedef enum {Numeric = 0, Character} DataType;

typedef struct
{
  __IO DataType	Type;
  __IO uint8_t PIDByte;
  __IO uint8_t FineByte;
  __IO char *Format;
  float (*Equation0)(uint8_t* data);
  char* (*Equation1)(uint8_t* data);
}DSControlTypeDef;

/**************************宏定义**************************************/
#define  DSTotalX       151
#define  NONE           0
/**************************外部变量************************************/
extern const DSControlTypeDef DSControl[DSTotalX];
/**************************计算函数声明********************************/
float Formula000(u8 *p);
char* Formula001(u8 *p);
char* Formula002(u8 *p);
char* Formula003(u8 *p);
char* Formula004(u8 *p);
char* Formula005(u8 *p);
char* Formula006(u8 *p);
char* Formula007(u8 *p);
char* Formula008(u8 *p);
char* Formula009(u8 *p);
char* Formula010(u8 *p);
char* Formula011(u8 *p);
char* Formula012(u8 *p);
char* Formula013(u8 *p);
char* Formula014(u8 *p);
char* Formula015(u8 *p);
char* Formula016(u8 *p);
char* Formula017(u8 *p);
float Formula018(u8 *p);
float Formula019(u8 *p);
float Formula020(u8 *p);
float Formula021(u8 *p);
float Formula022(u8 *p);
float Formula023(u8 *p);
float Formula024(u8 *p);
float Formula025(u8 *p);
char* Formula026(u8 *p);
char* Formula027(u8 *p);
float Formula028(u8 *p);
float Formula029(u8 *p);
char* Formula030(u8 *p);
char* Formula031(u8 *p);
char* Formula032(u8 *p);
float Formula033(u8 *p);
float Formula034(u8 *p);
float Formula035(u8 *p);
float Formula036(u8 *p);
float Formula037(u8 *p);
float Formula038(u8 *p);
float Formula039(u8 *p);
float Formula040(u8 *p);
float Formula041(u8 *p);
float Formula042(u8 *p);
float Formula043(u8 *p);
char* Formula044(u8 *p);
float Formula045(u8 *p);
float Formula046(u8 *p);
float Formula047(u8 *p);
float Formula048(u8 *p);
float Formula049(u8 *p);
float Formula050(u8 *p);
#endif

