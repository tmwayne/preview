//
// -----------------------------------------------------------------------------
// data-mmap.c
// -----------------------------------------------------------------------------
//
// Implementation of Data_T instance to load data using mmap
//
// Copyright © 2021 Tyler Wayne
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdlib.h>   // exit, EXIT_FAILURE
#include <string.h>   // strdup, strlen
#include <stdint.h>   // uint8_t

#include <sys/mman.h> // mmap, MAP_FAILED
#include <sys/stat.h> // fstat, open
#include <fcntl.h>    // O_RDONLY
#include <unistd.h>   // ssize_t, write, close, sysconf
#include "mem.h"      // NEW0, CALLOC, FREE

#include "deque.h"
#include "frame.h"
#include "errorcodes.h"

#define TOK_OK    1
#define TOK_EOL   2
#define TOK_EOF   4
#define TOK_ERR   8

typedef struct mmap_args {
  char *ptr;
} *mmap_args;

static int get_tok_r(char **tok, int *nbytes, char *str, const char delim, 
  char **saveptr, ssize_t len) {

  // TODO: check for trailing whitespace in file

  if (*saveptr == NULL) *saveptr = str;

  char *field = *saveptr;
  // TODO: make this a parameter
  int in_quote = 0;

  *nbytes = 1;
  for ( int i=0; ; i++, (*nbytes)++) {

    switch(field[i]) {
      case '\0':
        if (*saveptr+i >= str+len) return TOK_EOF;
        *saveptr += (i+1);
        *tok = field;
        return TOK_OK;

      case '\n':
        // field[i] = '\0';
        *saveptr += (i+1);
        *tok = field;
        return TOK_EOL;

      case '"':
        in_quote = in_quote ? 0 : 1;
        break;

      default:
        if (field[i]==delim && !in_quote) {
          // field[i] = '\0';
          *saveptr += (i+1);
          *tok = field;
          return TOK_OK;
        }
    }
  }

  return TOK_ERR;

}

// TODO: enable getting multiple rows at a time to make this more efficient

static int get_row(Data_T data, char **buf, int row, int col_start, int col_end) {

  // TODO: currently only supports one row lookahead
  // TODO: check if line is whitespace

  int parsed = 0;
  int *row_offsets = data->row_offsets;
  ssize_t len = data->st_size;
  char *ptr = ((mmap_args) data->args)->ptr;
  int err, nbytes, total_bytes;
  char *tok, *saveptr = NULL;

  int i = 0, icol = 0; // indexing values

  // Input checks
  if (data->ncols && col_end >= data->ncols) return E_DTA_COL_OOB;
  if (data->ncols && col_end == -1) col_end = data->ncols-1;

  if (row > 0) {
    if (row_offsets[row] == len) return E_DTA_EOF;
    if (!row_offsets[row]) return E_DTA_ROW_OOB; 
  }

  if (row_offsets[row+1]) {
    len = row_offsets[row+1] - row_offsets[row];
    parsed = 1;
  } 

  if (!data->ncols && parsed) return E_DTA_BAD_INPUT;

  if (parsed) {

    while (1) {
      err = get_tok_r(&tok, &nbytes, ptr+row_offsets[row], 
        data->delim, &saveptr, len);
      if (err & (TOK_ERR | TOK_EOF))
        return E_DTA_PARSE_ERROR; 
      if (icol >= col_start && icol <= col_end) buf[i++] = tok;
      if (icol == col_end) break;
      icol++;
    }

  } else {

    total_bytes = row_offsets[row];
    while (1) {
      err = get_tok_r(&tok, &nbytes, ptr+row_offsets[row], 
        data->delim, &saveptr, len);
      if (err == TOK_ERR) return E_DTA_PARSE_ERROR; // EOL, EOF are okay
      else if (err == TOK_EOF) break; 

      if (icol >= col_start && (icol <= col_end || col_end == -1)) 
        buf[i++] = tok;
      total_bytes += nbytes;

      if (err == TOK_EOL) {
        if (!data->ncols) data->ncols = icol+1;
        if (icol != data->ncols-1) return E_DTA_MISSING_FIELD;
        else break;
      }

      icol++;
    }
    row_offsets[row + 1] = total_bytes;
    data->nrows++;
  }

  return E_OK;

}

static int get_col(Data_T data, char **buf, int col, int row_start, int row_end) {

  int *row_offsets = data->row_offsets;
  char *ptr = ((mmap_args) data->args)->ptr;
  int err, nbytes; // , total_bytes;
  char *tok = NULL, *saveptr;

  if (col > data->ncols-1) return E_DTA_COL_OOB;
  if (row_end > data->nrows-1) return E_DTA_ROW_OOB;
  
  for (int irow=row_start, i=0; irow<=row_end; irow++, i++) {
    saveptr = NULL;
    int len = row_offsets[irow+1] - row_offsets[irow];
    for (int icol=0; icol <= col; icol++) {

      err = get_tok_r(&tok, &nbytes, ptr+row_offsets[irow], 
        data->delim, &saveptr, len);
      if (err & (TOK_ERR | TOK_EOF))
        return E_DTA_PARSE_ERROR;

    }
    buf[i] = tok;
  }

  return E_OK;

}

static int data_open(Data_T data) {

  mmap_args _args = data->args;

  int fd = open(data->path, O_RDONLY);
  if (fd < 0) return E_DTA_FILE_ERROR;

  struct stat statbuf;
  if (fstat(fd, &statbuf) < 0) return E_DTA_FILE_ERROR;

  long PAGESIZE = sysconf(_SC_PAGESIZE);
  if (statbuf.st_size % PAGESIZE == 0) return E_DTA_FILE_ERROR;

  char *ptr = mmap(
    NULL,                   // address
    statbuf.st_size,        // length
    PROT_READ,              // protection
    MAP_SHARED,             // privacy
    fd,                     // file descriptor
    0                       // offset
  );

  close(fd);

  if (ptr == MAP_FAILED) return E_DTA_FILE_ERROR;

  data->st_size = statbuf.st_size;
  _args->ptr = ptr;

  return E_OK;

}

static int data_close(Data_T data) {

  char *ptr = ((mmap_args) data->args)->ptr;

  if (munmap(ptr, data->st_size) != 0)
    return E_DTA_RESOURCE_ERROR;
  
  return E_OK;

}

// Our tokens from mmap won't be NULL-terminated.
// Instead they'll be terminated by either the delimiter
// or the newline. We avoid writing to the mmap-ed file
// by printing based on these new terminators
static int mvaddntok(int row, int col, const char *tok, 
  int n, char delim) { // this needs to be args
  
  for (int c=0; c<n; c++) {
    if (*tok == delim || *tok == '\n') return 1; // TODO: figure out return value
    else mvaddch(row, col, *tok);
    col++, tok++;
  }

  return 1;

}

Data_T Data_mmap_init(char *path, char delim) {

  if (!delim || !strlen(path)) return NULL;

  Data_T data;
  NEW0(data);

  data->path = path;
  data->delim = delim;
  data->row_offsets = calloc(MAX_ROWS, sizeof(int));
  data->open = data_open;
  data->get_col = get_col;
  data->get_row = get_row;;
  data->mvaddntok = mvaddntok;
  data->close = data_close;

  // Nothing needs to be done to free nodes inside the frame
  data->free_node = NULL;

  mmap_args args;
  NEW0(args);

  data->args = args;

  return data;

}

void Data_mmap_free(Data_T *data) {

  assert(data && *data && (*data)->args); 
  assert((*data)->row_offsets);

  FREE((*data)->row_offsets);
  FREE((*data)->args);
  FREE(*data);

}
