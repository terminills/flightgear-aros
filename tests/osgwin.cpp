/* osgwin: first test of the AROS window class of OpenSceneGraph. Opens a viewer window with three lit shapes
 * and a line of text, turns them, and prints the GL strings, the frame rate and every input event.
 * Options: --frames N (stop after N frames), --size WxH, --undecorated, --nocursor, --warp (move the pointer
 * to the middle of the window at frame 100). */

#include <osg/Geode>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Timer>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>
#include <osgText/Text>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include <cstdio>
#include <cstdlib>
#include <cstring>

USE_GRAPHICSWINDOW()

class EventLog : public osgGA::GUIEventHandler
{
    public:
        EventLog() : _moves(0) {}

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter&)
        {
            switch (ea.getEventType())
            {
                case osgGA::GUIEventAdapter::KEYDOWN:
                    printf("key down 0x%x plain 0x%x mod 0x%x\n", ea.getKey(), ea.getUnmodifiedKey(), ea.getModKeyMask());
                    break;
                case osgGA::GUIEventAdapter::KEYUP:
                    printf("key up 0x%x plain 0x%x mod 0x%x\n", ea.getKey(), ea.getUnmodifiedKey(), ea.getModKeyMask());
                    break;
                case osgGA::GUIEventAdapter::PUSH:
                    printf("push button %d at %.0f,%.0f\n", ea.getButton(), ea.getX(), ea.getY());
                    break;
                case osgGA::GUIEventAdapter::RELEASE:
                    printf("release button %d at %.0f,%.0f\n", ea.getButton(), ea.getX(), ea.getY());
                    break;
                case osgGA::GUIEventAdapter::SCROLL:
                    printf("scroll %d\n", (int)ea.getScrollingMotion());
                    break;
                case osgGA::GUIEventAdapter::RESIZE:
                    printf("resize %dx%d at %d,%d\n", ea.getWindowWidth(), ea.getWindowHeight(),
                           ea.getWindowX(), ea.getWindowY());
                    break;
                case osgGA::GUIEventAdapter::CLOSE_WINDOW:
                    printf("close window\n");
                    break;
                case osgGA::GUIEventAdapter::MOVE:
                case osgGA::GUIEventAdapter::DRAG:
                    if (++_moves % 50 == 1) printf("pointer %.0f,%.0f (%u moves)\n", ea.getX(), ea.getY(), _moves);
                    break;
                default:
                    return false;
            }
            fflush(stdout);
            return false;
        }

    private:
        unsigned int _moves;
};

struct PrintGL : public osg::Camera::DrawCallback
{
    PrintGL() : _done(false) {}

    virtual void operator()(osg::RenderInfo&) const
    {
        if (_done) return;
        _done = true;
        printf("GL_VENDOR %s\n", (const char*)glGetString(GL_VENDOR));
        printf("GL_RENDERER %s\n", (const char*)glGetString(GL_RENDERER));
        printf("GL_VERSION %s\n", (const char*)glGetString(GL_VERSION));
        fflush(stdout);
    }

    mutable bool _done;
};

struct Turn : public osg::NodeCallback
{
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform* mt = static_cast<osg::MatrixTransform*>(node);
        const double t = nv->getFrameStamp() ? nv->getFrameStamp()->getSimulationTime() : 0.0;

        mt->setMatrix(osg::Matrix::rotate(t * 0.8, osg::Vec3(0.0f, 0.0f, 1.0f)));
        traverse(node, nv);
    }
};

static osg::Geode* shape(osg::Shape* s, const osg::Vec4& colour)
{
    osg::ShapeDrawable* d = new osg::ShapeDrawable(s);
    osg::Geode* g = new osg::Geode;

    d->setColor(colour);
    g->addDrawable(d);
    return g;
}

int main(int argc, char** argv)
{
    int frames = 0, width = 800, height = 600;
    bool decorated = true, nocursor = false, warp = false;

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--size") && i + 1 < argc) sscanf(argv[++i], "%dx%d", &width, &height);
        else if (!strcmp(argv[i], "--undecorated")) decorated = false;
        else if (!strcmp(argv[i], "--nocursor")) nocursor = true;
        else if (!strcmp(argv[i], "--warp")) warp = true;
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    osg::ref_ptr<osg::MatrixTransform> turning = new osg::MatrixTransform;

    turning->addChild(shape(new osg::Box(osg::Vec3(-2.2f, 0.0f, 0.0f), 1.6f), osg::Vec4(0.9f, 0.3f, 0.2f, 1.0f)));
    turning->addChild(shape(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 1.0f), osg::Vec4(0.2f, 0.7f, 0.3f, 1.0f)));
    turning->addChild(shape(new osg::Cone(osg::Vec3(2.2f, 0.0f, -0.5f), 0.9f, 1.8f), osg::Vec4(0.2f, 0.4f, 0.9f, 1.0f)));
    turning->setUpdateCallback(new Turn);
    root->addChild(turning.get());

    osg::ref_ptr<osgText::Text> text = new osgText::Text;
    text->setText("OpenSceneGraph on AROS");
    text->setCharacterSize(0.6f);
    text->setAxisAlignment(osgText::Text::XZ_PLANE);
    text->setAlignment(osgText::Text::CENTER_CENTER);
    text->setPosition(osg::Vec3(0.0f, 0.0f, 2.2f));
    text->setColor(osg::Vec4(1.0f, 1.0f, 0.6f, 1.0f));
    osg::ref_ptr<osg::Geode> textGeode = new osg::Geode;
    textGeode->addDrawable(text.get());
    textGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    root->addChild(textGeode.get());

    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;

    traits->x = 60;
    traits->y = 40;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = decorated;
    traits->supportsResize = true;
    traits->doubleBuffer = true;
    traits->windowName = "osgwin";

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (!gc.valid())
    {
        printf("no graphics context\n");
        return 20;
    }

    osg::Camera* camera = viewer.getCamera();
    camera->setGraphicsContext(gc.get());
    camera->setViewport(new osg::Viewport(0, 0, width, height));
    camera->setProjectionMatrixAsPerspective(30.0, (double)width / height, 1.0, 1000.0);
    camera->setClearColor(osg::Vec4(0.10f, 0.12f, 0.20f, 1.0f));
    camera->setDrawBuffer(GL_BACK);
    camera->setReadBuffer(GL_BACK);
    camera->setInitialDrawCallback(new PrintGL);

    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setSceneData(root.get());
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.addEventHandler(new EventLog);
    viewer.realize();

    osgViewer::GraphicsWindow* window = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());
    if (!window || !viewer.isRealized())
    {
        printf("window not realized\n");
        return 20;
    }
    printf("window %s realized\n", window->className());
    if (nocursor) window->setCursor(osgViewer::GraphicsWindow::NoCursor);
    fflush(stdout);

    const osg::Timer_t start = osg::Timer::instance()->tick();
    osg::Timer_t mark = start;
    int n = 0;

    while (!viewer.done() && (frames == 0 || n < frames))
    {
        viewer.frame();
        ++n;
        if (warp && n == 100)
        {
            window->requestWarpPointer(width / 2.0f, height / 2.0f);
            printf("pointer warped to %d,%d\n", width / 2, height / 2);
        }
        if (n % 200 == 0)
        {
            const osg::Timer_t now = osg::Timer::instance()->tick();

            printf("frame %d: %.1f fps\n", n, 200.0 / osg::Timer::instance()->delta_s(mark, now));
            fflush(stdout);
            mark = now;
        }
    }

    printf("%d frames in %.1f s\n", n, osg::Timer::instance()->delta_s(start, osg::Timer::instance()->tick()));
    return 0;
}
