/*******************************************************************************
* 说    明: MAIN 主入口函数.
* 版    本: 1.0
* 作    者:
* - Dhorz <honorx@outlook.com>
* 更改记录:
* v1.0 - 初始版本
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

#define _DEBUG (0U)

#if (_DEBUG > 0)
#define DEBUG(fmt, ...) printf (fmt,##__VA_ARGS__)
#else  /* if (_DEBUG > 0) */
#define DEBUG(fmt, ...)
#endif /* if (_DEBUG > 0) */

#define FILE_MERGE_COUNTS_MAX (8U)
#define FILE_PATH_LENGTH_MAX  (256U)
#define FILE_BUFFER_SIZE      (1 * 1024 * 1024)
#define FILE_SIZE_MAX         (256 * 1024 * 1024)

typedef struct {
  char     path[FILE_PATH_LENGTH_MAX];
  uint32_t size;
  int32_t  offset;
} BinFile_t;

/**
 * 短选项配置:
 * > 无冒号: 选项
 * > 单冒号: 选项带参数
 * > 双冒号: 选项带可选参数
 */
static const char const *short_option = "s:p:h";

/* 长选项配置 */
static const struct option long_option[] =
{
  /* *INDENT-OFF* */
  "size", required_argument, NULL, 's', /* 输出文件大小 */
  "pad",  required_argument, NULL, 'p', /* 填充字节     */
  "help", no_argument,       NULL, 'h', /* 使用信息     */
  NULL,   no_argument,       NULL, 0,
  /* *INDENT-ON* */
};

/*******************************************************************************
 * 名    称: print_usage
 * 参    数: 无
 * 返 回 值: 无
 * 描    述: 打印使用信息
 * 注意事项: 无
 ******************************************************************************/
static void print_usage (void) {
  printf ("Usage: MergeBin [--size=<size>] [--pad=<padbyte>] <offset1@input1>\r\n");
  printf ("                  <offset2@input2> ... <offset8@input8> [<output>]\r\n");
  printf ("\r\n");
  printf ("Options:\r\n");
  printf ("    size (-s)    Output file size, default is not specified;\r\n");
  printf ("    pad (-f)     Pad free space with specified byte;\r\n");
  printf ("    help (-h)    Print usage massages;\r\n");
  printf ("\r\n");
  printf ("Notice:\r\n");
  printf ("    Offset should be specified address or '+' (follow previous file);\r\n");
  printf ("    Offset start at 0x00000000 by default;\r\n");
  printf ("    Offset 'MUST' from low to hith;\r\n");
  printf ("\r\n");
  printf ("Example:\r\n");
  printf ("    MergeBin 0x00000000@boot.bin 0x00002000@app.bin [firmware.bin]\r\n");
  printf ("    MergeBin +@boot.bin +@app.bin [firmware.bin]\r\n");
  printf ("\r\n");
  printf ("binary merge tools v1.0.0 write by Dhorz <honorx@outlook.com>\r\n");
  printf ("\r\n");
}

/*******************************************************************************
 * 名    称: main
 * 参    数:
 *   argc - 参数个数
 *   argv - 参数值
 * 返 回 值: 执行状态
 * 描    述: MAIN 函数
 * 注意事项: 无
 ******************************************************************************/
int main (int argc, char **argv) {
  int       res;
  int       options_index;
  FILE     *file_input;
  FILE     *file_output;
  char      file_output_path[FILE_PATH_LENGTH_MAX];
  int32_t   file_output_size;
  uint8_t   file_output_padbyte;
  BinFile_t file_input_list[FILE_MERGE_COUNTS_MAX];
  uint32_t  file_input_index;

  if (argc <= 1) {
    /* 输出使用信息 */
    print_usage ();

    res = 0;
    goto _END_OF_PROCESS;
  }

  res                 = 0;
  file_input          = NULL;
  file_output         = NULL;
  file_output_size    = 0;
  file_output_padbyte = 0xFF;
  file_input_index    = 0;
  file_output_path[0] = '\0';

  /* 文件列表初始化 */
  for (int i = 0; i < FILE_MERGE_COUNTS_MAX; i++) {
    file_input_list[i].path[0] = '\0';
    file_input_list[i].size    = 0;
    file_input_list[i].offset  = -1;
  }

  options_index = 0;

  /* 选项参数解析 */
  for ( ;; ) {
    int     idx = getopt_long (argc, argv, short_option, long_option, NULL);
    char   *stop_ptr;
    int32_t value;

    if (idx < 0) {
      break;
    }

    switch (idx) {
    case 's' :
      /* 输出文件大小 */
      errno = 0;
      value = strtoul (optarg, &stop_ptr, 0);

      if ((optarg == stop_ptr) || (errno != 0) || (value <= 0) || (value > FILE_SIZE_MAX * FILE_MERGE_COUNTS_MAX)) {
        /* 输入的不是一个数值或者是数值溢出 */
        printf ("Illegal or unrecognized size options !\r\n");

        res = -1;
        goto _END_OF_PROCESS;
      }

      file_output_size = value;

      break;

    case 'p' :
      /* 填充字节 */
      errno = 0;
      value = strtoul (optarg, &stop_ptr, 0);

      if ((optarg == stop_ptr) || (errno != 0) || (value < 0x00) || (value > 0xFF)) {
        /* 输入的不是一个数值或者是数值溢出 */
        printf ("Illegal or unrecognized padbyte options !\r\n");

        res = -1;
        goto _END_OF_PROCESS;
      }

      file_output_padbyte = value;

      break;

    case 'h' :
      /* 输出使用信息 */
      print_usage ();

      res = 0;
      goto _END_OF_PROCESS;

    default :
      printf ("Illegal or unrecognized options !\r\n");

      res = -1;
      goto _END_OF_PROCESS;
    }
  }

  /* 非选项参数解析 */
  options_index = optind;

  for (int i = options_index; i < argc; i++) {
    char *stop_ptr;
    char *at_ptr;

    /* 尝试识别输入文件的路径以及写入偏移地址 */
    at_ptr = strchr (argv[i], '@');

    /* 获取偏移地址 */
    if (at_ptr != NULL) {
      uint32_t value;

      errno = 0;
      value = strtoul (argv[i], &stop_ptr, 0);

      if ((argv[i] != stop_ptr) && (errno == 0)) {
        /* 指定 OFFSET */
        file_input_list[file_input_index].offset = value;
      } else if (argv[i][0] == '+') {
        /* OFFSET 连续 */
        file_input_list[file_input_index].offset = -1;
      } else {
        /* OFFSET 非法, 输入的不是一个数值或者是数值溢出 */
        printf ("Illegal or unrecognized offset options !\r\n");

        res = -1;
        goto _END_OF_PROCESS;
      }

      /* 拷贝输入文件路径 */
      strncpy (&file_input_list[file_input_index].path[0], (const char *)(at_ptr + 1), FILE_PATH_LENGTH_MAX - 1);
      file_input_list[file_input_index].path[FILE_PATH_LENGTH_MAX - 1] = '\0';
      file_input_index++;
    } else {
      /* 拷贝输出文件路径 */
      strncpy (&file_output_path[0], (const char *)(argv[i]), FILE_PATH_LENGTH_MAX - 1);
      file_output_path[FILE_PATH_LENGTH_MAX - 1] = '\0';
      break;
    }
  }

  /* 未指定输出文件名时使用默认值 */
  if (file_output_path[0] == '\0') {
    strncpy (&file_output_path[0], (const char *)("output.bin"), FILE_PATH_LENGTH_MAX - 1);
    file_output_path[FILE_PATH_LENGTH_MAX - 1] = '\0';
  }

  /* 输出文件 */
  file_output = fopen (file_output_path, "wb");

  if (file_output == NULL) {
    printf ("Can not open output file: %s !\r\n", file_output_path);

    res = -1;
    goto _END_OF_PROCESS;
  }

  /* 移动游标到文件开头 */
  fseek (file_output, 0L, SEEK_SET);

  for (int i = 0; i < file_input_index; i++) {
    file_input = fopen (file_input_list[i].path, "rb");

    if (file_input == NULL) {
      printf ("Can not open input file: %s !\r\n", file_input_list[i].path);

      res = -1;
      goto _END_OF_PROCESS;
    }

    /* 获取文件大小 */
    fseek (file_input, 0L, SEEK_END);
    file_input_list[i].size = ftell (file_input);
    fseek (file_input, 0L, SEEK_SET);

    if (file_input_list[i].size > FILE_SIZE_MAX) {
      printf ("File too big: %s !\r\n", file_input_list[i].path);

      res = -1;
      goto _END_OF_PROCESS;
    }

    /* 设置偏移位置 */
    if (file_input_list[i].offset >= 0) {
      uint32_t cursor = ftell (file_output);

      /* 填充游标到 OFFSET 之间的空间 */
      if (cursor < file_input_list[i].offset) {
        uint32_t cnts = file_input_list[i].offset - cursor;

        while (cnts-- > 0) {
          fputc (file_output_padbyte, file_output);
        }
      }

      /* 移动游标到指定 Offset */
      fseek (file_output, file_input_list[i].offset, SEEK_SET);
    }

    /* 循环读取输入文件并写入输出文件 */
    for ( ;; ) {
      uint8_t buffer[FILE_BUFFER_SIZE];

      int32_t len = fread (buffer, 1, FILE_BUFFER_SIZE, file_input);

      if (len > 0) {
        fwrite (buffer, 1, len, file_output);
      } else {
        if (file_input != NULL) {
          fclose (file_input);
        }

        break;
      }
    }
  }

  /* 若指定输出文件大小时尝试填充文件 */
  if (file_output_size != 0) {
    uint32_t cursor = ftell (file_output);

    /* 填充空闲空间 */
    if (cursor <= file_output_size) {
      uint32_t cnts = file_output_size - cursor;

      while (cnts-- > 0) {
        fputc (file_output_padbyte, file_output);
      }
    } else {
      /* 输出警告但不截断文件 */
      printf ("WARNING: Output file size is larger than expected !\r\n");
    }
  }

  res = 0;

_END_OF_PROCESS:
  if (file_input != NULL) {
    fclose (file_input);
  }

  if (file_output != NULL) {
    fclose (file_output);
  }

  return (res);
}
