/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TDENGINE_QUERYUTIL_H
#define TDENGINE_QUERYUTIL_H

#include <libs/function/function.h>
#include "tbuffer.h"
#include "tcommon.h"
#include "tpagedbuf.h"

#define SET_RES_WINDOW_KEY(_k, _ori, _len, _uid)     \
  do {                                               \
    assert(sizeof(_uid) == sizeof(uint64_t));        \
    *(uint64_t *)(_k) = (_uid);                      \
    memcpy((_k) + sizeof(uint64_t), (_ori), (_len)); \
  } while (0)

#define SET_RES_EXT_WINDOW_KEY(_k, _ori, _len, _uid, _buf)             \
  do {                                                                 \
    assert(sizeof(_uid) == sizeof(uint64_t));                          \
    *(void **)(_k) = (_buf);                                             \
    *(uint64_t *)((_k) + POINTER_BYTES) = (_uid);                      \
    memcpy((_k) + POINTER_BYTES + sizeof(uint64_t), (_ori), (_len));   \
  } while (0)


#define GET_RES_WINDOW_KEY_LEN(_l) ((_l) + sizeof(uint64_t))
#define GET_RES_EXT_WINDOW_KEY_LEN(_l) ((_l) + sizeof(uint64_t) + POINTER_BYTES)

#define GET_TASKID(_t)  (((SExecTaskInfo*)(_t))->id.str)

typedef struct SGroupResInfo {
  int32_t index;
  SArray* pRows;      // SArray<SResKeyPos>
  int32_t position;
} SGroupResInfo;

typedef struct SResultRow {
  int32_t       pageId;      // pageId & rowId is the position of current result in disk-based output buffer
  int32_t       offset:29;   // row index in buffer page
  bool          startInterp; // the time window start timestamp has done the interpolation already.
  bool          endInterp;   // the time window end timestamp has done the interpolation already.
  bool          closed;      // this result status: closed or opened
  uint32_t      numOfRows;   // number of rows of current time window
  STimeWindow   win;
  struct SResultRowEntryInfo pEntryInfo[];  // For each result column, there is a resultInfo
//  char         *key;         // start key of current result row
} SResultRow;

typedef struct SResultRowPosition {
  int32_t pageId;
  int32_t offset;
} SResultRowPosition;

typedef struct SResKeyPos {
  SResultRowPosition pos;
  uint64_t  groupId;
  char      key[];
} SResKeyPos;

typedef struct SResultRowInfo {
  SResultRowPosition *pPosition;  // todo remove this
  int32_t      size;       // number of result set
  int32_t      capacity;   // max capacity
  SResultRowPosition cur;
  SList*       openWindow;
} SResultRowInfo;

struct SqlFunctionCtx;

size_t getResultRowSize(struct SqlFunctionCtx* pCtx, int32_t numOfOutput);
int32_t initResultRowInfo(SResultRowInfo* pResultRowInfo, int32_t size);
void    cleanupResultRowInfo(SResultRowInfo* pResultRowInfo);

int32_t numOfClosedResultRows(SResultRowInfo* pResultRowInfo);
void    closeAllResultRows(SResultRowInfo* pResultRowInfo);

void    initResultRow(SResultRow *pResultRow);
void    closeResultRow(SResultRow* pResultRow);
bool    isResultRowClosed(SResultRow* pResultRow);

struct SResultRowEntryInfo* getResultCell(const SResultRow* pRow, int32_t index, const int32_t* offset);

static FORCE_INLINE SResultRow *getResultRow(SDiskbasedBuf* pBuf, SResultRowInfo *pResultRowInfo, int32_t slot) {
  ASSERT(pResultRowInfo != NULL && slot >= 0 && slot < pResultRowInfo->size);
  SResultRowPosition* pos = &pResultRowInfo->pPosition[slot];

  SFilePage*  bufPage = (SFilePage*) getBufPage(pBuf, pos->pageId);
  SResultRow* pRow = (SResultRow*)((char*)bufPage + pos->offset);
  return pRow;
}

static FORCE_INLINE SResultRow *getResultRowByPos(SDiskbasedBuf* pBuf, SResultRowPosition* pos) {
  SFilePage*  bufPage = (SFilePage*) getBufPage(pBuf, pos->pageId);
  SResultRow* pRow = (SResultRow*)((char*)bufPage + pos->offset);
  return pRow;
}

void    initGroupedResultInfo(SGroupResInfo* pGroupResInfo, SHashObj* pHashmap, int32_t order);
void    initMultiResInfoFromArrayList(SGroupResInfo* pGroupResInfo, SArray* pArrayList);

void    cleanupGroupResInfo(SGroupResInfo* pGroupResInfo);
bool    hashRemainDataInGroupInfo(SGroupResInfo* pGroupResInfo);

bool    incNextGroup(SGroupResInfo* pGroupResInfo);
int32_t getNumOfTotalRes(SGroupResInfo* pGroupResInfo);

#endif  // TDENGINE_QUERYUTIL_H
