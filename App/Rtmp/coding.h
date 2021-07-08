#ifndef __CODING_H__
#define __CODING_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */

/******************************
pout_buf:out PCM
pin_buf:in G711A
***********************************/
int g711a_decode(void *pout_buf, int *pout_len, const void *pin_buf, const int  in_len);

/******************************
pout_buf:out G711A
pin_buf:in PCM
***********************************/
int g711a_encode(void *pout_buf, int *pout_len, const void *pin_buf, const int in_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */

#endif
