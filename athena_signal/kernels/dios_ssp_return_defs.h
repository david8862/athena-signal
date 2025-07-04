/* Copyright (C) 2017 Beijing Didi Infinity Technology and Development Co.,Ltd.
All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef _DIOS_SSP_RETURN_DEFS_H_
#define _DIOS_SSP_RETURN_DEFS_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum
{
    OK_AUDIO_PROCESS,
    ERROR_AUDIO_PROCESS,
    ERROR_AEC,
    ERROR_VAD,
    ERROR_MVDR,
    ERROR_GSC,
    ERROR_DOA,
    ERROR_HPF,
    ERROR_NS,
    ERROR_AGC
} FUN_RETURN;


#ifdef  __cplusplus
}
#endif

#endif  /* _DIOS_SSP_RETURN_DEFS_H_ */

