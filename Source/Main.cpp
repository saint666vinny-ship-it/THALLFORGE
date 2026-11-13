#include <JuceHeader.h>
#include <SoundTouch.h>
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>

class ThallAudio final : public juce::Component, private juce::AudioIODeviceCallback {
public:
    ThallAudio() {
        setSize(900, 620);
        deviceManager.initialise(1, 2, nullptr, true);
        deviceManager.addAudioCallback(this);
        setup(pitch, -24, 24, 1, " semitones", 0);
        setup(fine, -100, 100, 1, " cents", 0);
        setup(mix, 0, 100, 1, " %", 100);
        setup(drive, 0, 10, 0.01, "", 5);
        setup(gate, -90, -10, 1, " dB", -60);
        setup(output, -24, 12, 0.1, " dB", 0);
        for (auto* s : {&pitch,&fine,&mix,&drive,&gate,&output}) addAndMakeVisible(*s);
        pitch.onValueChange=[this]{semitones.store(pitch.getValue());};
        fine.onValueChange=[this]{cents.store(fine.getValue());};
        mix.onValueChange=[this]{wet.store(mix.getValue()/100.0);};
        drive.onValueChange=[this]{driveAmount.store(drive.getValue());};
        gate.onValueChange=[this]{gateDb.store(gate.getValue());};
        output.onValueChange=[this]{gain.store(std::pow(10.0,output.getValue()/20.0));};
    }
    ~ThallAudio() override { deviceManager.removeAudioCallback(this); deviceManager.closeAudioDevice(); }
    void audioDeviceAboutToStart(juce::AudioIODevice* d) override {
        const double sr=d?d->getCurrentSampleRate():44100.0;
        shifter.clear(); shifter.setSampleRate((unsigned int)sr); shifter.setChannels(2);
        shifter.setSetting(SETTING_USE_QUICKSEEK,1); shifter.setSetting(SETTING_USE_AA_FILTER,1);
        shifter.setSetting(SETTING_SEQUENCE_MS,40); shifter.setSetting(SETTING_SEEKWINDOW_MS,15); shifter.setSetting(SETTING_OVERLAP_MS,8);
        inputBuffer.resize(8192); outputBuffer.resize(8192); envelope.assign(2,0.0f);
    }
    void audioDeviceStopped() override { shifter.clear(); }
    void audioDeviceIOCallback(const float** in,int ni,float** out,int no,int n) override {
        if(no<1||n<1)return; const int ch=juce::jmin(2,no); const size_t needed=(size_t)n*ch;
        if(inputBuffer.size()<needed){inputBuffer.resize(needed);outputBuffer.resize(needed);}
        shifter.setPitchSemiTones((float)(semitones.load()+cents.load()/100.0));
        for(int i=0;i<n;++i)for(int c=0;c<ch;++c){int ic=ni>1?c:0;inputBuffer[(size_t)i*ch+c]=(ni>0&&in[ic])?in[ic][i]:0.0f;}
        shifter.putSamples(inputBuffer.data(),(unsigned int)n);
        unsigned int got=juce::jmin((unsigned int)n,shifter.numSamples()); if(got)shifter.receiveSamples(outputBuffer.data(),got);
        const float w=(float)wet.load(), threshold=(float)std::pow(10.0,gateDb.load()/20.0), amount=1.0f+(float)driveAmount.load()*2.2f, g=(float)gain.load();
        for(int i=0;i<n;++i)for(int c=0;c<ch;++c){int ic=ni>1?c:0;float dry=(ni>0&&in[ic])?in[ic][i]:0.0f;float shifted=(unsigned int)i<got?outputBuffer[(size_t)i*ch+c]:0.0f;float x=dry*(1-w)+shifted*w;float& e=envelope[(size_t)c];e=std::max(std::abs(x),e*0.9992f);if(e<threshold)x*=0.08f;x=std::tanh(x*amount)/std::tanh(amount);out[c][i]=x*g;}
        for(int c=ch;c<no;++c)juce::FloatVectorOperations::clear(out[c],n);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff101114));g.setColour(juce::Colour(0xffe0e0e4));g.setFont(juce::Font(juce::FontOptions(34.0f,juce::Font::bold)));g.drawText("THALLFORGE",30,20,500,48,juce::Justification::left);
        g.setColour(juce::Colour(0xffa7aab3));g.setFont(juce::Font(14.0f));g.drawText("REAL-TIME GUITAR PROCESSOR / PITCH FIRST",33,65,600,22,juce::Justification::left);
        g.setColour(juce::Colour(0xff272a31));g.fillRoundedRectangle(24,105,852,250,12);g.fillRoundedRectangle(24,375,852,210,12);
        g.setColour(juce::Colour(0xfff04b55));g.setFont(juce::Font(18.0f,juce::Font::bold));g.drawText("PITCH SHIFTER",44,120,260,28,juce::Justification::left);g.drawText("THALL AMP / OUTPUT",44,390,300,28,juce::Justification::left);
        g.setColour(juce::Colour(0xffa7aab3));g.setFont(juce::Font(12.0f));g.drawText("SoundTouch pitch processing adds latency. ASIO is recommended.",32,590,840,18,juce::Justification::left);
    }
    void resized() override {pitch.setBounds(50,170,245,145);fine.setBounds(325,170,245,145);mix.setBounds(600,170,245,145);drive.setBounds(70,440,220,105);gate.setBounds(340,440,220,105);output.setBounds(610,440,220,105);}
private:
    static void setup(juce::Slider& s,double lo,double hi,double step,const juce::String& suffix,double value){s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,110,24);s.setRange(lo,hi,step);s.setValue(value);s.setTextValueSuffix(suffix);s.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xfff04b55));}
    juce::AudioDeviceManager deviceManager;juce::Slider pitch,fine,mix,drive,gate,output;soundtouch::SoundTouch shifter;std::vector<float> inputBuffer,outputBuffer,envelope;
    std::atomic<double> semitones{0},cents{0},wet{1},driveAmount{5},gateDb{-60},gain{1};
};
class ThallApp final:public juce::JUCEApplication{public:const juce::String getApplicationName()override{return "THALLFORGE";}const juce::String getApplicationVersion()override{return "0.1.0";}void initialise(const juce::String&)override{window.reset(new MainWindow(getApplicationName()));}void shutdown()override{window=nullptr;}class MainWindow final:public juce::DocumentWindow{public:explicit MainWindow(juce::String n):DocumentWindow(n,juce::Colour(0xff101114),allButtons){setUsingNativeTitleBar(true);setContentOwned(new ThallAudio(),true);centreWithSize(getWidth(),getHeight());setResizable(true,true);setVisible(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}};private:std::unique_ptr<MainWindow> window;};
START_JUCE_APPLICATION(ThallApp)
