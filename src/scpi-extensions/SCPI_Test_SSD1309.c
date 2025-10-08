#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scpi/scpi.h"
#include "scpi-def.h"

#include <FreeRTOS.h>
#include "driver_ssd1309_advance.h"

extern ssd1309_handle_t gs_handle;

scpi_result_t SCPI_Display_state(scpi_t * context)
{
    (void) context;
    uint32_t param1;

    if (!SCPI_ParamUInt32(context, &param1, FALSE)) {
        return SCPI_RES_ERR;
    }

    switch (param1) {
    case 1:
        if(ssd1309_advance_display_on() != 0){
            return SCPI_RES_ERR;
        }
        break;
    
    default:
        if(ssd1309_advance_display_off() != 0){
            return SCPI_RES_ERR;
        }
        break;
    }

    return SCPI_RES_OK;
}

scpi_result_t SCPI_Display_clear(scpi_t * context)
{
    (void) context;

    if(ssd1309_advance_clear() != 0){
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t SCPI_Display_write(scpi_t * context)
{
    (void) context;

    char buffer[256];
    int size = 0;
    
    if (!SCPI_ParamCopyText(context, buffer, 256, &size, TRUE)) {
        return SCPI_RES_ERR;
    }

    if(ssd1309_advance_clear() != 0){
        return SCPI_RES_ERR;
    }

    if(ssd1309_advance_string(1, 1, buffer, size, 1, SSD1309_FONT_16) != 0){
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t SCPI_Display_point(scpi_t * context)
{
    (void) context;

    uint32_t param1, param2;

    if (!SCPI_ParamUInt32(context, &param1, TRUE)) {
        return SCPI_RES_ERR;
    }

    if (!SCPI_ParamUInt32(context, &param2, TRUE)) {
        return SCPI_RES_ERR;
    }

    if(ssd1309_advance_write_point(param1, param2, 1) != 0){
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Display_square(scpi_t * context)
{
    (void) context;

    uint32_t param[4] = {0,0,0,0};
    size_t count = 0;

    if(ssd1309_advance_clear() != 0){
        return SCPI_RES_ERR;
    }

    if (!SCPI_ParamArrayUInt32(context, param, 4, &count, SCPI_FORMAT_ASCII, TRUE)) {
        return SCPI_RES_ERR;
    }

    // bool good = (param[0] > 0 && param[0] < 64)  &&
    //             (param[1] > 0 && param[1] < 64)  &&
    //             (param[2] > 0 && param[2] < 128) &&
    //             (param[3] > 0 && param[3] < 128);
    
    // if(good == FALSE){
    //     return SCPI_RES_ERR;
    // }
    
    if(ssd1309_advance_rect(param[0], param[1], param[2], param[3],  1) != 0){
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Display_picture(scpi_t * context)
{
    (void) context;

    const int width = 64;
    const int height = 128;

    uint8_t **image = (uint8_t **)malloc(height * sizeof(uint8_t *));
    for (int i = 0; i < height; i++) {
        image[i] = (uint8_t *)calloc(width, sizeof(uint8_t));
    }

    for (int i = 0; i < width; i++) {
        image[0][i] = 1;
        image[height - 1][i] = 1;
    }
    for (int i = 0; i < height; i++) {
        image[i][0] = 1;
        image[i][width - 1] = 1;
    }
    if(ssd1309_clear(&gs_handle) != 0){
        free(image);
        return SCPI_RES_ERR;
    }
    
    // size_t img_size = width * height;
    // if (!SCPI_ParamArrayUInt8(context, &image[0][0], 1*64, &received, SCPI_FORMAT_ASCII, TRUE)) {
    //     free(image);
    //     return SCPI_RES_ERR;
    // }
    
    // if(ssd1309_advance_picture(0, 0, width-1, height-1, &image[0][0]) != 0){
    //     free(image);
    //     return SCPI_RES_ERR;
    // }

    // uint8_t image[width][height];
    // uint8_t *ptr = &image[0][0];
    // uint32_t offset = 0;
    
    // printf("RECEIVE\n");
    // while (offset < img_size) {
    //     // offset += xStreamBufferReceive(UartStream, (ptr + offset), 64, pdMS_TO_TICKS(100));
    //     *(ptr + offset) = fgetc (stdin);
    //     offset++;
    //     if(offset % 64 == 0){
    //         printf("\n");
    //     }
    // }
    // printf("COMPLETE\n");

    for (uint8_t x = 0; x < height; x++)                                            /* write x */
    {
        for (uint8_t y = 0; y < width; y++)                                         /* write y */
        {
            if (ssd1309_gram_write_point( &gs_handle, x, y, image[x][y]) != 0)      /* draw point */
            {
                free(image);
                return SCPI_RES_ERR;                                                /* return error */
            }
        }
    }
    
    ssd1309_gram_update(&gs_handle);

    free(image);
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Display_slow(scpi_t * context)
{
    (void) context;

    size_t count = 0;
    const uint8_t width = 128;
    const uint8_t height = 64;

    for (uint8_t i = 0; i < width; i++) {
        for (uint8_t j = 0; j < height; j++) {
            if(ssd1309_gram_write_point( &gs_handle, i, j, 1) != 0){
                return SCPI_RES_ERR;
            }
            ssd1309_gram_update(&gs_handle);
        }
    }

    for (uint8_t j = 0; j < height; j++) {
        for (uint8_t i = 0; i < width; i++) {
            if(ssd1309_gram_write_point( &gs_handle, i, j, 0) != 0){
                return SCPI_RES_ERR;
            }
            ssd1309_gram_update(&gs_handle);
        }
    }

    
    return SCPI_RES_OK;
}
