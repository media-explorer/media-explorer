#ifndef __MEX_GRILO_H__
#define __MEX_GRILO_H__

#include <grilo.h>

#include <mex/mex-content.h>

G_BEGIN_DECLS

void mex_grilo_init (int *p_argc, char **p_argv[]);

void mex_grilo_set_media_content_metadata (GrlMedia           *media,
                                           MexContentMetadata  mex_key,
                                           const gchar        *value);

void mex_grilo_update_content_from_media (MexContent *content,
                                          GrlMedia   *media);

G_END_DECLS

#endif /* __MEX_GRILO_H__ */
