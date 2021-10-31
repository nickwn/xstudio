#pragma once

struct source_pos 
{
    const char* name;
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};