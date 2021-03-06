//
// -----------------------------------------------------------------------------
// test_deque.c
// -----------------------------------------------------------------------------
//
// Tyler Wayne © 2021
//

#include <stdio.h>
#include "error.h"
#include "minunit.h"
#include "frame.h"

int tests_run = 0;

// Frame_T Frame_init(int col_width, int max_cols, int max_rows, int headers);
static char *test_Frame_init_valid() {
  Frame_T frame = Frame_init(1, 1, 1, 0);
  mu_assert("Frame_init returned NULL", frame);
}

static char *test_Frame_init_NULL_colwidth0() {
  Frame_T frame = Frame_init(0, 1, 1, 0);
  mu_assert("Frame_init didn't return NULL on col_width 0", !frame);
}

static char *test_Frame_init_NULL_maxcol0() {
  Frame_T frame = Frame_init(1, 0, 1, 0);
  mu_assert("Frame_init didn't return NULL on max_cols 0", !frame);
}

static char *test_Frame_init_NULL_maxrow0() {
  Frame_T frame = Frame_init(1, 1, 0, 1);
  mu_assert("Frame_init didn't return NULL on max_rows 0", !frame);
}
  
// void Frame_free(Frame_T *frame);
static char *test_Frame_free_valid() {
  Frame_T frame = Frame_init(1, 1, 1, 0);
  Frame_free(&frame, NULL, NULL);
  mu_assert("Frame didn't set frame to NULL", !frame);
}

static char *test_Frame_free_throw_NULL_arg() {
  unsigned char pass = 0;
  TRY Frame_free(NULL, NULL, NULL);
  EXCEPT (Assert_Failed) pass = 1;
  END_TRY;
  mu_assert("Frame_free didn't throw error when passed NULL", pass);
}

static char *test_Frame_free_throw_NULL_frame() {
  unsigned char pass = 0;
  Frame_T frame = NULL;
  TRY Frame_free(&frame, NULL, NULL);
  EXCEPT (Assert_Failed) pass = 1;
  END_TRY;
  mu_assert("Frame_free didn't throw error when passed NULL frame", pass);
}

// int Frame_print(Frame_T frame);
static char *test_Frame_print_throw_NULL_frame() {
  unsigned char pass = 0;
  Frame_T frame = NULL;
  Data_T data = NULL;
  TRY Frame_print(frame, data, O_FRM_DATA);
  EXCEPT (Assert_Failed) pass = 1;
  END_TRY;
  mu_assert("Frame_print didn't throw error when passed NULL frame", pass);
}

static char *test_Frame_print_throw_length0_data() {
  unsigned char pass = 0;
  Frame_T frame = Frame_init(1, 1, 1, 0);
  Data_T data = NULL;
  TRY Frame_print(frame, data, O_FRM_DATA);
  EXCEPT (Assert_Failed) pass = 1;
  END_TRY;
  mu_assert("Frame_print didn't throw error when passed length 0 data", pass);
}

static char* run_all_tests() {

  char *(*all_tests[])() = {
    test_Frame_init_valid,
    test_Frame_init_NULL_colwidth0,
    test_Frame_init_NULL_maxcol0,
    test_Frame_init_NULL_maxrow0,
    test_Frame_free_valid,
    test_Frame_free_throw_NULL_arg,
    test_Frame_free_throw_NULL_frame,
    test_Frame_print_throw_NULL_frame,
    test_Frame_print_throw_length0_data,
    NULL
  };

  // Returns message of first failing test
  mu_run_all(all_tests);
    
  return 0;
}

int main(int argc, char** argv) {
  char* result = run_all_tests();
  if (result != 0) printf("%s\n", result);
  else printf("ALL TESTS PASSED\n");
  printf("Tests run: %d\n", tests_run);
  return result != 0;
}
