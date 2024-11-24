/*
  ==============================================================================

    MarkerSlider.h
    Created: 1 Oct 2024 4:28:04pm
    Author:  liann77

  ==============================================================================
*/

#pragma once
// MarkerSlider.h

#include <JuceHeader.h>
#include "Marker.h"

#include <functional>  // 确保引入 functional 头文件以使用 std::function

class MarkerSlider : public juce::Slider
{
public:
    MarkerSlider()
    {
        setInterceptsMouseClicks(true, true);
    }

    // 添加一个标记
    void addMarker(double position)
    {
        markers.emplace_back(position);
        repaint();  // 添加标记后重绘
    }

    // 清除所有标记
    void clearMarkers()
    {
        markers.clear();
        repaint();  // 清除标记后重绘
    }

    // 获取所有标记
    const std::vector<Marker>& getMarkers() const { return markers; }

    // 标记拖动后的回调
    std::function<void()> onMarkersChanged;  // 声明 onMarkersChanged 回调
    // 获取最近被拖动的标记索引
    int getLastDraggedMarkerIndex() const { return lastDraggedMarkerIndex; }

protected:
    std::vector<Marker> markers;      // 存储所有标记
    int draggingMarkerIndex = -1;     // 当前正在拖动的标记索引
    int lastDraggedMarkerIndex = -1;  // 记录最近被拖动的标记索引


    // 绘制标记
    // 绘制标记
    void paint(juce::Graphics& g) override
    {
        juce::Slider::paint(g);  // 先绘制滑块本身

        for (const auto& marker : markers)
        {
            double sliderRangeStart = getMinimum();
            double sliderRangeEnd = getMaximum();
            double sliderProportion = (marker.position - sliderRangeStart) / (sliderRangeEnd - sliderRangeStart);
            sliderProportion = juce::jlimit(0.0, 1.0, sliderProportion);
            int x = static_cast<int>(sliderProportion * getWidth());
            
            // 定义线条的起始和结束位置
            int lineYStart = 0;  // 线条的起点（顶部）
            int lineYEnd = getHeight();  // 线条的终点（底部）
            
            // 根据标记状态设置颜色
            if (marker.isTriggered)
                g.setColour(juce::Colours::green);
            else if (marker.isDragging)
                g.setColour(juce::Colours::orange);
            else
                g.setColour(juce::Colours::red);

            // 绘制线条
            float lineThickness = 2.0f; // 线条粗细，可以根据需要调整
            g.drawLine((float)x, (float)lineYStart, (float)x, (float)lineYEnd, lineThickness);
        }
    }


    // 处理鼠标点击
    void mouseDown(const juce::MouseEvent& event) override
    {
        for (size_t i = 0; i < markers.size(); ++i)
        {
            const Marker& marker = markers[i];
            double sliderRangeStart = getMinimum();
            double sliderRangeEnd = getMaximum();
            double sliderProportion = (marker.position - sliderRangeStart) / (sliderRangeEnd - sliderRangeStart);
            sliderProportion = juce::jlimit(0.0, 1.0, sliderProportion);
            float x = static_cast<float>(sliderProportion * getWidth());
            float y = static_cast<float>(getHeight() / 2.0);

            juce::Rectangle<float> markerArea(x - 5.0f, y - 10.0f, 10.0f, 15.0f);
            if (markerArea.contains(event.position))
            {
                draggingMarkerIndex = static_cast<int>(i);
                lastDraggedMarkerIndex = draggingMarkerIndex;  // 记录被拖动的标记索引
                markers[i].isDragging = true;
                return;
                }
        }
        juce::Slider::mouseDown(event);
    }

    // 处理鼠标拖动
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (draggingMarkerIndex >= 0 && draggingMarkerIndex < static_cast<int>(markers.size()))
        {
            double proportion = static_cast<double>(event.position.x) / getWidth();
            proportion = juce::jlimit(0.0, 1.0, proportion);
            double newMarkerTime = proportion * (getMaximum() - getMinimum()) + getMinimum();

            markers[draggingMarkerIndex].position = newMarkerTime;
            repaint();

            // 如果设置了 onMarkersChanged 回调，调用它
            if (onMarkersChanged)
                onMarkersChanged();
        }
        else
        {
            juce::Slider::mouseDrag(event);
        }
    }

    // 处理鼠标松开
    void mouseUp(const juce::MouseEvent& event) override
    {
        if (draggingMarkerIndex >= 0 && draggingMarkerIndex < static_cast<int>(markers.size()))
        {
            markers[draggingMarkerIndex].isDragging = false;
            draggingMarkerIndex = -1;
        }
        juce::Slider::mouseUp(event);
    }
};
