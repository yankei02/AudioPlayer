/*
  ==============================================================================

    Marker.h
    Created: 1 Oct 2024 4:27:50pm
    Author:  liann77

  ==============================================================================
*/
// Marker.h
#pragma once
//结构体marker，用来表示一个marker
struct Marker
{
    double position;     // 标记的位置，以秒为单位
    bool isDragging;     // 标记是否正在被拖动
    bool isTriggered;    // 标记是否已经被触发

    Marker(double pos)
        : position(pos), isDragging(false), isTriggered(false) {}
};
