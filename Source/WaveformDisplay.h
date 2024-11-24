/*
  ==============================================================================

    WaveformDisplay.h
    Created: 30 Sep 2024 9:53:18pm
    Author:  liann77

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// WaveformDisplay 类声明
class WaveformDisplay : public juce::Component,
                        public juce::ChangeListener
{
public:
    WaveformDisplay(juce::AudioThumbnail& thumbnailToUse);//绘制音频波形
    ~WaveformDisplay() override;

    void paint(juce::Graphics& g) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;//当 AudioThumbnail 数据发生变化时，接收通知并更新显示。
    void setPosition(double position);//设置当前播放位置，用于在波形上显示播放指示线。

    void mouseDown(const juce::MouseEvent& event) override;//处理鼠标点击事件，支持用户点击波形来改变播放位置。
    std::function<void(double)> onPositionChanged;//当用户点击波形改变播放位置时，调用此回调函数通知外部组件。
    void setCurrentPosition(double newPosition)
        {
            currentPosition = newPosition;
            repaint();
        }
private:
    juce::AudioThumbnail& audioThumbnail;//用于绘制音频波形，引用juce::AudioThumbnail
    double currentPosition = 0.0;//当前播放位置

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
