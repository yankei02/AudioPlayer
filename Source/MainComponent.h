#pragma once

#include <JuceHeader.h>
#include <juce_audio_devices/juce_audio_devices.h>   // 包含音频设备管理的组件
#include <juce_audio_formats/juce_audio_formats.h>   // 包含音频格式管理组件
#include <poppler/glib/poppler.h>  // Poppler C API
#include <glib.h>                   // GLib 头文件，用于 GError 等类型
#include <cairo/cairo.h>            // Cairo 库
#include "WaveformDisplay.h"
#include "MarkerSlider.h"
#include "Marker.h"


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class GrayLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrayLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::grey);
        setColour(juce::Slider::trackColourId, juce::Colours::grey);
        setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // 绘制轨道
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRect(x, y + height / 2 - 2, width, 4);

        // 绘制滑块
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillEllipse(sliderPos - 6, y + height / 2 - 6, 12, 12);
    }
};


class MainComponent  : public juce::AudioAppComponent, public juce::FileDragAndDropTarget, public juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;
    
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    // 文件拖拽处理
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // 定时器，用于更新播放进度条
    void timerCallback() override;
    
    void setCurrentPosition(double newPosition)
        {
            currentPosition = newPosition;
            waveformDisplay.setCurrentPosition(newPosition);
            progressSlider.setValue(newPosition, juce::dontSendNotification);
        }
    void recalculateAndAddMarkers() ;
    //marker file sace/load
    void saveMarkerPositions(const juce::File& file);
    void loadMarkerPositions(const juce::File& file);

private:
    //==============================================================================
    // Audio components
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioThumbnailCache thumbnailCache;      // 声明 thumbnailCache
    juce::AudioThumbnail audioThumbnail;           // 声明 audioThumbnail
    WaveformDisplay waveformDisplay;               // 声明 waveformDisplay

    // GUI components
    juce::Slider progressSlider;
    juce::Label audioFileNameLabel;
    juce::Label pdfFileNameLabel;
    juce::TextButton playButton{ "Play" };
    juce::TextButton pauseButton{ "Pause" };
    juce::TextButton nextButton{ "Next" };  // 新增的“Next”按钮
    juce::TextButton beforeButton{ "Before" };  // 新增的“Before”按钮
    juce::ImageComponent pdfImageComponent;
    juce::ImageComponent nextPagePreview;

    // PDF handling
    void loadAndDisplayPDF(const juce::File& pdfFile);
    void renderPdfPageToComponent(PopplerPage* pdfPage, juce::ImageComponent& component, int pageIndex);


    // Poppler document
    PopplerDocument* pdfDoc = nullptr;
    int currentPageIndex = 0;
    int nextPageIndex = currentPageIndex + 1;
    int totalNumPages = 0;
    juce::String pdfDocFileName;  // 存储 PDF 文件名
    juce::Label audioPositionLabel;  // 新增，用于显示音频播放秒数的标签
    juce::Label audioLengthLabel; //显示音频长度的标签

    // 音频播放相关成员（如 AudioTransportSource 等）
    void changeListenerCallback(juce::ChangeBroadcaster* source);
    double currentPosition = 0.0; // 以秒为单位
    GrayLookAndFeel grayLookAndFeel;
    
    // 新增的 MarkerSlider
    MarkerSlider markerSlider; // 新的滑块用于显示标记

    // 映射用于跟踪标记是否已触发
    std::map<const Marker*, bool> markerTriggeredFlags;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
    // markerSave button
    juce::TextButton saveMarkersButton;
    std::unique_ptr<juce::FileChooser> fileChooser; // 添加这一行
    // 使用 std::map 或 std::unordered_map 作为缓存
    std::unordered_map<int, juce::Image> renderedPageCache;
};
