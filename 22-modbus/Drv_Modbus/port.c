/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

#include <stdio.h>
#include "port.h"

/*
 * __aeabi_assert 函数实现
 * 当断言失败时被调用，输出错误信息并进入死循环
 */
void __aeabi_assert(const char *expr, const char *file, int line)
{
    /* 输出断言失败信息 */
    printf("**************************************************\n");
    printf("*** ASSERTION FAILED ***\n");
    printf("Expression: %s\n", expr);
    printf("File: %s\n", file);
    printf("Line: %d\n", line);
    printf("**************************************************\n");

    /* 进入死循环，便于调试 */
    while(1) {
        /* 触发调试断点 */
        __asm volatile("bkpt 0");

        /* 如果没有调试器连接，这里可能会继续执行 */
        /* 可以在这里添加 LED 闪烁等调试代码 */
    }
}
