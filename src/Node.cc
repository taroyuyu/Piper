#include <QStyleOptionGraphicsItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>

#include <algorithm>

#include "Node.h"
#include "Link.h"
#include "AttributeMember.h"

namespace piper
{
    constexpr int attributeHeight = 30;
    constexpr int baseHeight = 35;
    constexpr int baseWidth  = 250;
    QColor const default_background {80, 80, 80, 255};
    QColor const attribute_brush    {60, 60, 60, 255};
    QColor const attribute_brush_alt{70, 70, 70, 255};

    QList<Node*> Node::items_{};
    QList<Node *> const& Node::items()
    {
        return items_;
    }
    
    void Node::resetStagesColor()
    {
        for (auto& node : items())
        {
            node->bgBrush_.setColor(default_background);
            node->update();
        }
    }
    
    void Node::updateStagesColor(QString const& stage, QColor const& color)
    {
        for (auto& node : items())
        {
            if (node->stage_ == stage)
            {
                node->bgBrush_.setColor(color);
                node->update();
            }
        }
    }

    Node::Node(QString const& type, QString const& name, QString const& stage)
        : QGraphicsItem(nullptr)
        , name_{name}
        , type_{type}
        , stage_{stage}
        , width_{baseWidth}
        , height_{baseHeight}
        , attributes_{}
    {  
        // Configure item behavior.
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemIsFocusable);
        
        createStyle();
        
        // add this to the items list;
        Node::items_.append(this);
    }

    Node::~Node()
    {
        // Remove this from the items list.
        Node::items_.removeAll(this);
    }

    void Node::highlight( Attribute* emitter)
    {
        for (auto& attr : attributes_)
        {
            if (attr == emitter)
            {
                // special case: do not change emitter mode.
                continue;
            }
            
            if (attr->accept(emitter))
            {
                attr->setMode(DisplayMode::highlight);
            }
            else
            {
                attr->setMode(DisplayMode::minimize);
            }
            attr->update();
        }
    }

    void Node::unhighlight()
    {
        for (auto& attr : attributes_)
        {
            attr->setMode(DisplayMode::normal);
            attr->update();
        }
    }

    void Node::addAttribute(AttributeInfo const& info)
    {
        constexpr QRect boundingRect{0, 0, baseWidth-2, attributeHeight};
        
        Attribute* attr;
        switch (info.type)
        {
            case AttributeInfo::Type::input:
            {
                attr = new AttributeInput (this, info.name, info.dataType, boundingRect);
                break;
            }
            case AttributeInfo::Type::output:
            {
                attr = new AttributeOutput (this, info.name, info.dataType, boundingRect);
                break;
            }
            case AttributeInfo::Type::member:
            {
                attr = new AttributeMember (this, info.name, info.dataType, boundingRect);
                break;
            }
        }
        attr->setPos(1, 17 + attributeHeight * attributes_.size());
        if (attributes_.size() % 2)
        {
            attr->setBackgroundBrush(attrBrush_);
        }
        else
        {
            attr->setBackgroundBrush(attrAltBrush_);
        }
        height_ += attributeHeight;
        attributes_.append(attr);
    }

    void Node::createStyle()
    {
        qint32 border = 2;

        bgBrush_.setStyle(Qt::SolidPattern);
        bgBrush_.setColor(default_background);

        pen_.setStyle(Qt::SolidLine);
        pen_.setWidth(border);
        pen_.setColor({50, 50, 50, 255});

        penSel_.setStyle(Qt::SolidLine);
        penSel_.setWidth(border);
        penSel_.setColor({170, 80, 80, 255});

        textPen_.setStyle(Qt::SolidLine);
        textPen_.setColor({255, 255, 255, 255});

        textFont_ = QFont("Noto", 12, QFont::Bold);
        setName(name_);
        
        attrBrush_.setStyle(Qt::SolidPattern);
        attrBrush_.setColor(attribute_brush);
        attrAltBrush_.setStyle(Qt::SolidPattern);
        attrAltBrush_.setColor(attribute_brush_alt);
    }

    QRectF Node::boundingRect() const
    {
        return QRect(0, 0, width_, height_).united(textRect_);
    }

    void Node::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        
        // Base shape.
        painter->setBrush( bgBrush_ );
        
        if (isSelected())
        {
            painter->setPen(penSel_);
        }
        else
        {
            painter->setPen(pen_);
        }
        
        qint32 radius = 10;
        painter->drawRoundedRect(0, 0, width_, height_, radius, radius);
        
        // Label.
        painter->setPen(textPen_);
        painter->setFont(textFont_);
        painter->drawText(textRect_, Qt::AlignCenter, name_);
    }

    void Node::mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        // Force selected node on top layer
        for (auto& item : scene()->items())
        {
            if (item->zValue() > 1)
            {
                item->setZValue(1);
            }
        }
        setZValue(2);
        
        QGraphicsItem::mousePressEvent(event);
    }

    void Node::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
    {
        for (auto& attr : attributes_)
        {
            attr->refresh(); // let the attribute refresh their data if required.
        }
        
        QGraphicsItem::mouseMoveEvent(event);
    }
    
    void Node::setName(QString const& name)
    {
        name_ = name;
        
        QFontMetrics metrics(textFont_);
        qint32 text_width = metrics.boundingRect(name_).width() + 14;
        qint32 text_height = metrics.boundingRect(name_).height() + 14;
        qint32 margin = (text_width - width_) * 0.5;
        textRect_ = QRect(-margin, -text_height, text_width, text_height);
    }

    void Node::keyPressEvent(QKeyEvent* event)
    {
        constexpr qreal moveFactor = 5;
        if ((event->key() == Qt::Key::Key_Up) and (event->modifiers() == Qt::NoModifier))
        {
            moveBy(0, -moveFactor);
        }
        if ((event->key() == Qt::Key::Key_Down) and (event->modifiers() == Qt::NoModifier))
        {
            moveBy(0, moveFactor);
        }
        if ((event->key() == Qt::Key::Key_Left) and (event->modifiers() == Qt::NoModifier))
        {
            moveBy(-moveFactor, 0);
        }
        if ((event->key() == Qt::Key::Key_Right) and (event->modifiers() == Qt::NoModifier))
        {
            moveBy(moveFactor, 0);
        }
    }

    
    Link* connect(QString const& from, QString const& out, QString const& to, QString const& in)
    {
        QList<Node*>::const_iterator nodeFrom = std::find_if(Node::items().begin(), Node::items().end(),
            [&](Node const* node) { return (node->name_ == from); }
        );
        QList<Node*>::const_iterator nodeTo = std::find_if(Node::items().begin(), Node::items().end(),
            [&](Node const* node) { return (node->name_ == to); }
        );
        
        Attribute* attrOut{nullptr};
        for (auto& attr : (*nodeFrom)->attributes_)
        {
            if (attr->name() == out)
            {
                attrOut = attr;
                break;
            }
        }
        
        Attribute* attrIn{nullptr};
        for (auto& attr : (*nodeTo)->attributes_)
        {
            if (attr->name() == in)
            {
                attrIn = attr;
                break;
            }
        }
        
        if (attrIn == nullptr) 
        {
            qDebug() << "Can't find attribute" << in << "(in) in the node" << to;
            std::abort();
        }

        if (attrOut == nullptr)
        {
            qDebug() << "Can't find attribute" << out << "(out) in the node" << from;
            std::abort();
        }
        
        if (not attrIn->accept(attrOut))
        {
            qDebug() << "Can't connect attribute" << from << "to attribute" << to;
            std::abort();
        }
        
        Link* link= new Link;
        link->connectFrom(attrOut);
        link->connectTo(attrIn);
        return link;
    }
}
