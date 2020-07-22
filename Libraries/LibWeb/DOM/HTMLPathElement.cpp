/*
 * Copyright (c) 2020, Matthew Olsson <matthewcolsson@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/StringBuilder.h>
#include <LibGfx/Path.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/Event.h>
#include <LibWeb/DOM/HTMLPathElement.h>
#include <ctype.h>

//#define PATH_DEBUG

namespace Web {

PathDataParser::PathDataParser(const String& source)
    : m_source(source)
{
}

Vector<PathInstruction> PathDataParser::parse()
{
    parse_whitespace();
    while (!done())
        parse_drawto();
    if (!m_instructions.is_empty() && m_instructions[0].type != PathInstructionType::Move)
        ASSERT_NOT_REACHED();
    return m_instructions;
}

void PathDataParser::parse_drawto() {
    if (match('M') || match('m')) {
        parse_moveto();
    } else if (match('Z') || match('z')) {
        parse_closepath();
    } else if (match('L') || match('l')) {
        parse_lineto();
    } else if (match('H') || match('h')) {
        parse_horizontal_lineto();
    } else if (match('V') || match('v')) {
        parse_vertical_lineto();
    } else if (match('C') || match('c')) {
        parse_curveto();
    } else if (match('S') || match('s')) {
        parse_smooth_curveto();
    } else if (match('Q') || match('q')) {
        parse_quadratic_bezier_curveto();
    } else if (match('T') || match('t')) {
        parse_smooth_quadratic_bezier_curveto();
    } else if (match('A') || match('a')) {
        parse_elliptical_arc();
    }
}

void PathDataParser::parse_moveto()
{
    bool absolute = consume() == 'M';
    parse_whitespace();
    for (auto& pair : parse_coordinate_pair_sequence())
        m_instructions.append({ PathInstructionType::Move, absolute, pair });
}

void PathDataParser::parse_closepath()
{
    bool absolute = consume() == 'Z';
    m_instructions.append({ PathInstructionType::ClosePath, absolute, {} });
}

void PathDataParser::parse_lineto()
{
    bool absolute = consume() == 'L';
    parse_whitespace();
    for (auto& pair : parse_coordinate_pair_sequence())
        m_instructions.append({ PathInstructionType::Line, absolute, pair });
}

void PathDataParser::parse_horizontal_lineto()
{
    bool absolute = consume() == 'H';
    parse_whitespace();
    m_instructions.append({ PathInstructionType::HorizontalLine, absolute, parse_coordinate_sequence() });
}

void PathDataParser::parse_vertical_lineto()
{
    bool absolute = consume() == 'V';
    parse_whitespace();
    m_instructions.append({ PathInstructionType::VerticalLine, absolute, parse_coordinate_sequence() });
}

void PathDataParser::parse_curveto()
{
    bool absolute = consume() == 'C';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::Curve, absolute, parse_coordinate_pair_triplet() });
        parse_whitespace();
        if (!match_number())
            break;
    }
}

void PathDataParser::parse_smooth_curveto()
{
    bool absolute = consume() == 'S';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::SmoothCurve, absolute, parse_coordinate_pair_double() });
        parse_whitespace();
        if (!match_number())
            break;
    }
}

void PathDataParser::parse_quadratic_bezier_curveto()
{
    bool absolute = consume() == 'Q';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::QuadraticBezierCurve, absolute, parse_coordinate_pair_double() });
        parse_whitespace();
        if (!match_number())
            break;
    }
}

void PathDataParser::parse_smooth_quadratic_bezier_curveto()
{
    bool absolute = consume() == 'T';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::SmoothQuadraticBezierCurve, absolute, parse_coordinate_pair_double() });
        parse_whitespace();
        if (!match_number())
            break;
    }
}

void PathDataParser::parse_elliptical_arc()
{
    bool absolute = consume() == 'A';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::EllipticalArc, absolute, parse_elliptical_arg_argument() });
        parse_whitespace();
        if (!match_number())
            break;
    }
}

float PathDataParser::parse_coordinate()
{
    return parse_number();
}

Vector<float> PathDataParser::parse_coordinate_pair()
{
    Vector<float> coordinates;
    coordinates.append(parse_coordinate());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.append(parse_coordinate());
    return coordinates;
}

Vector<float> PathDataParser::parse_coordinate_sequence()
{
    Vector<float> sequence;
    while (true) {
        sequence.append(parse_coordinate());
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_comma_whitespace() && !match_number())
            break;
    }
    return sequence;
}

Vector<Vector<float>> PathDataParser::parse_coordinate_pair_sequence()
{
    Vector<Vector<float>> sequence;
    while (true) {
        sequence.append(parse_coordinate_pair());
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_comma_whitespace() && !match_number())
            break;
    }
    return sequence;
}

Vector<float> PathDataParser::parse_coordinate_pair_double()
{
    Vector<float> coordinates;
    coordinates.append(parse_coordinate_pair());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.append(parse_coordinate_pair());
    return coordinates;
}

Vector<float> PathDataParser::parse_coordinate_pair_triplet()
{
    Vector<float> coordinates;
    coordinates.append(parse_coordinate_pair());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.append(parse_coordinate_pair());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.append(parse_coordinate_pair());
    return coordinates;
}

Vector<float> PathDataParser::parse_elliptical_arg_argument()
{
    Vector<float> numbers;
    numbers.append(parse_number());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.append(parse_number());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.append(parse_number());
    parse_comma_whitespace();
    numbers.append(parse_flag());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.append(parse_flag());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.append(parse_coordinate_pair());

    return numbers;
}

void PathDataParser::parse_whitespace(bool must_match_once)
{
    bool matched = false;
    while (!done() && match_whitespace()) {
        consume();
        matched = true;
    }

        ASSERT(!must_match_once || matched);
}

void PathDataParser::parse_comma_whitespace()
{
    if (match(',')) {
        consume();
        parse_whitespace();
    } else {
        parse_whitespace(1);
        if (match(','))
            consume();
        parse_whitespace();
    }
}

float PathDataParser::parse_fractional_constant()
{
    StringBuilder builder;
    bool floating_point = false;

    while (!done() && isdigit(ch()))
        builder.append(consume());

    if (match('.')) {
        floating_point = true;
        builder.append('.');
        consume();
        while (!done() && isdigit(ch()))
            builder.append(consume());
    } else {
        ASSERT(builder.length() > 0);
    }

    if (floating_point)
        return strtof(builder.to_string().characters(), nullptr);
    return builder.to_string().to_int().value();
}

float PathDataParser::parse_number()
{
    bool negative = false;
    if (match('-')) {
        consume();
        negative = true;
    } else if (match('+')) {
        consume();
    }
    auto number = parse_fractional_constant();
    if (match('e') || match('E'))
        TODO();
    return negative ? number * -1 : number;
}

float PathDataParser::parse_flag()
{
    auto number = parse_number();
        ASSERT(number == 0 || number == 1);
    return number;
}

bool PathDataParser::match_whitespace() const
{
    if (done())
        return false;
    char c = ch();
    return c == 0x9 || c == 0x20 || c == 0xa || c == 0xc || c == 0xd;
}

bool PathDataParser::match_comma_whitespace() const
{
    return match_whitespace() || match(',');
}

bool PathDataParser::match_number() const
{
    return !done() && (isdigit(ch()) || ch() == '-' || ch() == '+');
}

HTMLPathElement::HTMLPathElement(Document& document, const FlyString& tag_name)
    : HTMLElement(document, tag_name)
{
}

#ifdef PATH_DEBUG
static void print_instruction(const PathInstruction& instruction)
{
    auto& data = instruction.data;

    switch (instruction.type) {
    case PathInstructionType::Move:
        dbg() << "Move (absolute: " << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); i += 2)
            dbg() << "    x=" << data[i] << ", y=" << data[i + 1];
        break;
    case PathInstructionType::ClosePath:
        dbg() << "ClosePath (absolute=" << instruction.absolute << ")";
        break;
    case PathInstructionType::Line:
        dbg() << "Line (absolute=" << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); i += 2)
            dbg() << "    x=" << data[i] << ", y=" << data[i + 1];
        break;
    case PathInstructionType::HorizontalLine:
        dbg() << "HorizontalLine (absolute=" << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); ++i)
            dbg() << "    x=" << data[i];
        break;
    case PathInstructionType::VerticalLine:
        dbg() << "VerticalLine (absolute=" << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); ++i)
            dbg() << "    y=" << data[i];
        break;
    case PathInstructionType::Curve:
        dbg() << "Curve (absolute=" << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); i += 6)
            dbg() << "    (x1=" << data[i] << ", y1=" << data[i + 1] << "), (x2=" << data[i + 2] << ", y2=" << data[i + 3] << "), (x=" << data[i + 4] << ", y=" << data[i + 5] << ")";
        break;
    case PathInstructionType::SmoothCurve:
        dbg() << "SmoothCurve (absolute: " << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); i += 4)
            dbg() << "    (x2=" << data[i] << ", y2=" << data[i + 1] << "), (x=" << data[i + 2] << ", y=" << data[i + 3] << ")";
        break;
    case PathInstructionType::QuadraticBezierCurve:
        dbg() << "QuadraticBezierCurve (absolute: " << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); i += 4)
            dbg() << "    (x1=" << data[i] << ", y1=" << data[i + 1] << "), (x=" << data[i + 2] << ", y=" << data[i + 3] << ")";
        break;
    case PathInstructionType::SmoothQuadraticBezierCurve:
        dbg() << "SmoothQuadraticBezierCurve (absolute: " << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); i += 2)
            dbg() << "    x=" << data[i] << ", y=" << data[i + 1];
        break;
    case PathInstructionType::EllipticalArc:
        dbg() << "EllipticalArc (absolute: " << instruction.absolute << ")";
        for (size_t i = 0; i < data.size(); i += 7)
            dbg() << "    (rx=" << data[i] << ", ry=" << data[i + 1] << ") x-axis-rotation=" << data[i + 2] << ", large-arc-flag=" << data[i + 3] << ", sweep-flag=" << data[i + 4] << ", (x=" << data[i + 5] << ", y=" << data[i + 6] << ")";
        break;
    case PathInstructionType::Invalid:
        dbg() << "Invalid (absolute: " << instruction.absolute << ")";
        break;
    }
}
#endif

void HTMLPathElement::parse_attribute(const FlyString& name, const String& value)
{
    HTMLElement::parse_attribute(name, value);
    if (name == "d")
        m_instructions = PathDataParser(value).parse();
}

void HTMLPathElement::paint(const SvgPaintingContext& context, Gfx::Painter& painter)
{
    Gfx::Path path;

    for (auto& instruction : m_instructions) {
        auto& absolute = instruction.absolute;
        auto& data = instruction.data;

#ifdef PATH_DEBUG
        print_instruction(instruction);
#endif

        switch (instruction.type) {
        case PathInstructionType::Move:
            if (absolute) {
                path.move_to({ data[0], data[1] });
            } else {
                ASSERT(!path.segments().is_empty());
                path.move_to(Gfx::FloatPoint { data[0], data[1] } + path.segments().last().point);
            }
            break;
        case PathInstructionType::ClosePath:
            path.close();
            break;
        case PathInstructionType::Line:
            if (absolute) {
                path.line_to({ data[0], data[1] });
            } else {
                ASSERT(!path.segments().is_empty());
                path.line_to(Gfx::FloatPoint { data[0], data[1] } + path.segments().last().point);
            }
            break;
        case PathInstructionType::HorizontalLine: {
            ASSERT(!path.segments().is_empty());
            auto last_point = path.segments().last().point;
            if (absolute) {
                path.line_to(Gfx::FloatPoint { data[0], last_point.y() });
            } else {
                path.line_to(Gfx::FloatPoint { data[0] + last_point.x(), last_point.y() });
            }
            break;
        }
        case PathInstructionType::VerticalLine: {
            ASSERT(!path.segments().is_empty());
            auto last_point = path.segments().last().point;
            if (absolute) {
                path.line_to(Gfx::FloatPoint{ last_point.x(), data[0] });
            } else {
                path.line_to(Gfx::FloatPoint{ last_point.x(), data[0] + last_point.y() });
            }
            break;
        }
        case PathInstructionType::QuadraticBezierCurve:
            if (absolute) {
                path.quadratic_bezier_curve_to({ data[0], data[1] }, { data[2], data[3] });
            } else {
                ASSERT(!path.segments().is_empty());
                auto last_point = path.segments().last().point;
                path.quadratic_bezier_curve_to({ data[0] + last_point.x(), data[1] + last_point.y() }, { data[2] + last_point.x(), data[3] + last_point.y() });
            }
            break;
        case PathInstructionType::Curve:
        case PathInstructionType::SmoothCurve:
        case PathInstructionType::SmoothQuadraticBezierCurve:
        case PathInstructionType::EllipticalArc:
            TODO();
        case PathInstructionType::Invalid:
            ASSERT_NOT_REACHED();
        }
    }

    painter.fill_path(path, context.fill_color, Gfx::Painter::WindingRule::EvenOdd);
    painter.stroke_path(path, context.stroke_color, context.stroke_width);
}

}