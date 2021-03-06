/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2001-2002 NaN Holding BV. All rights reserved. */

/** \file
 * \ingroup edtransform
 */

#include <stdlib.h>

#include "BLI_math.h"
#include "BLI_string.h"
#include "BLI_task.h"

#include "BKE_context.h"
#include "BKE_unit.h"

#include "ED_screen.h"

#include "UI_interface.h"

#include "BLT_translation.h"

#include "transform.h"
#include "transform_convert.h"
#include "transform_snap.h"

#include "transform_mode.h"

/* -------------------------------------------------------------------- */
/** \name Transform (Rotation - Trackball) Element
 * \{ */

/**
 * \note Small arrays / data-structures should be stored copied for faster memory access.
 */
struct TransDataArgs_Trackball {
  const TransInfo *t;
  const TransDataContainer *tc;
  const float axis[3];
  const float angle;
  float mat_final[3][3];
};

static void transdata_elem_trackball(const TransInfo *t,
                                     const TransDataContainer *tc,
                                     TransData *td,
                                     const float axis[3],
                                     const float angle,
                                     const float mat_final[3][3])
{
  float mat_buf[3][3];
  const float(*mat)[3] = mat_final;
  if (t->flag & T_PROP_EDIT) {
    axis_angle_normalized_to_mat3(mat_buf, axis, td->factor * angle);
    mat = mat_buf;
  }
  ElementRotation(t, tc, td, mat, t->around);
}

static void transdata_elem_trackball_fn(void *__restrict iter_data_v,
                                        const int iter,
                                        const TaskParallelTLS *__restrict UNUSED(tls))
{
  struct TransDataArgs_Trackball *data = iter_data_v;
  TransData *td = &data->tc->data[iter];
  if (td->flag & TD_SKIP) {
    return;
  }
  transdata_elem_trackball(data->t, data->tc, td, data->axis, data->angle, data->mat_final);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Transform (Rotation - Trackball)
 * \{ */

static void applyTrackballValue(TransInfo *t,
                                const float axis1[3],
                                const float axis2[3],
                                const float angles[2])
{
  float mat_final[3][3];
  float axis[3];
  float angle;
  int i;

  mul_v3_v3fl(axis, axis1, angles[0]);
  madd_v3_v3fl(axis, axis2, angles[1]);
  angle = normalize_v3(axis);
  axis_angle_normalized_to_mat3(mat_final, axis, angle);

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    if (tc->data_len < TRANSDATA_THREAD_LIMIT) {
      TransData *td = tc->data;
      for (i = 0; i < tc->data_len; i++, td++) {
        if (td->flag & TD_SKIP) {
          continue;
        }
        transdata_elem_trackball(t, tc, td, axis, angle, mat_final);
      }
    }
    else {
      struct TransDataArgs_Trackball data = {
          .t = t,
          .tc = tc,
          .axis = {UNPACK3(axis)},
          .angle = angle,
      };
      copy_m3_m3(data.mat_final, mat_final);

      TaskParallelSettings settings;
      BLI_parallel_range_settings_defaults(&settings);
      BLI_task_parallel_range(0, tc->data_len, &data, transdata_elem_trackball_fn, &settings);
    }
  }
}

static void applyTrackball(TransInfo *t, const int UNUSED(mval[2]))
{
  char str[UI_MAX_DRAW_STR];
  size_t ofs = 0;
  float axis1[3], axis2[3];
#if 0 /* UNUSED */
  float mat[3][3], totmat[3][3], smat[3][3];
#endif
  float phi[2];

  copy_v3_v3(axis1, t->persinv[0]);
  copy_v3_v3(axis2, t->persinv[1]);
  normalize_v3(axis1);
  normalize_v3(axis2);

  copy_v2_v2(phi, t->values);

  transform_snap_increment(t, phi);

  applyNumInput(&t->num, phi);

  copy_v2_v2(t->values_final, phi);

  if (hasNumInput(&t->num)) {
    char c[NUM_STR_REP_LEN * 2];

    outputNumInput(&(t->num), c, &t->scene->unit);

    ofs += BLI_snprintf_rlen(str + ofs,
                             sizeof(str) - ofs,
                             TIP_("Trackball: %s %s %s"),
                             &c[0],
                             &c[NUM_STR_REP_LEN],
                             t->proptext);
  }
  else {
    ofs += BLI_snprintf_rlen(str + ofs,
                             sizeof(str) - ofs,
                             TIP_("Trackball: %.2f %.2f %s"),
                             RAD2DEGF(phi[0]),
                             RAD2DEGF(phi[1]),
                             t->proptext);
  }

  if (t->flag & T_PROP_EDIT_ALL) {
    ofs += BLI_snprintf_rlen(
        str + ofs, sizeof(str) - ofs, TIP_(" Proportional size: %.2f"), t->prop_size);
  }

#if 0 /* UNUSED */
  axis_angle_normalized_to_mat3(smat, axis1, phi[0]);
  axis_angle_normalized_to_mat3(totmat, axis2, phi[1]);

  mul_m3_m3m3(mat, smat, totmat);

  /* TRANSFORM_FIX_ME */
  // copy_m3_m3(t->mat, mat);  /* used in gizmo. */
#endif

  applyTrackballValue(t, axis1, axis2, phi);

  recalcData(t);

  ED_area_status_text(t->area, str);
}

void initTrackball(TransInfo *t)
{
  t->mode = TFM_TRACKBALL;
  t->transform = applyTrackball;

  initMouseInputMode(t, &t->mouse, INPUT_TRACKBALL);

  t->idx_max = 1;
  t->num.idx_max = 1;
  t->snap[0] = DEG2RAD(5.0);
  t->snap[1] = DEG2RAD(1.0);

  copy_v3_fl(t->num.val_inc, t->snap[1]);
  t->num.unit_sys = t->scene->unit.system;
  t->num.unit_use_radians = (t->scene->unit.system_rotation == USER_UNIT_ROT_RADIANS);
  t->num.unit_type[0] = B_UNIT_ROTATION;
  t->num.unit_type[1] = B_UNIT_ROTATION;

  t->flag |= T_NO_CONSTRAINT;
}

/** \} */
