/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#ifndef __TARS_PROTOCOL_H_
#define __TARS_PROTOCOL_H_

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "tup/RequestF.h"
#include "tup/tup.h"
#include "servant/BaseF.h"
#include "util/tc_epoll_server.h"
#include "util/tc_network_buffer.h"

#if TARS_HTTP2
#include "nghttp2/nghttp2.h"
#endif

using namespace std;
using namespace tup;

namespace tars
{

#define TARS_NET_MIN_PACKAGE_SIZE 5
#define TARS_NET_MAX_PACKAGE_SIZE 1024*1024*10

template<typename T>
T net2host(T len)
{
    switch(sizeof(T))
    {
        case sizeof(uint8_t): return len;
        case sizeof(uint16_t): return ntohs(len);
        case sizeof(uint32_t): return ntohl(len);
    }
    assert(true);
    return 0;
}
//////////////////////////////////////////////////////////////////////
/**
 * 协议解析
 */
class AppProtocol
{
public:
    // /**
    //  * 解析协议
    //  * @param in, 目前的buffer
    //  * @param out, 一个完整的包
    //  *
    //  * @return int, 0表示没有接收完全, 1表示收到一个完整包
    //  */
    // static int parse(string &in, string &out)
    // {
    //     return parseLen<10000000>(in,out);
    // }

    /**
     * 解析协议
     * @param in, 目前的buffer
     * @param out, 一个完整的包
     *
     * @return int, 0表示没有接收完全, 1表示收到一个完整包
     */
    static TC_NetWorkBuffer::PACKET_TYPE parse(TC_NetWorkBuffer &in, vector<char> &out)
    {
        return TC_NetWorkBuffer::parseBinary4<TARS_NET_MIN_PACKAGE_SIZE, TARS_NET_MAX_PACKAGE_SIZE>(in, out);
    }

    /**
     * 解析协议
     * @param in, 目前的buffer
     * @param out, 一个完整的包
     *
     * @return int, 0表示没有接收完全, 1表示收到一个完整包
     */
    static TC_NetWorkBuffer::PACKET_TYPE  parseAdmin(TC_NetWorkBuffer &in, shared_ptr<TC_NetWorkBuffer::SendBuffer> &out);


    /**
     *
     * @param T
     * @param offset
     * @param netorder
     * @param in
     * @param out
     * @return int
     */
    template<size_t offset, typename T, bool netorder>
    static TC_NetWorkBuffer::PACKET_TYPE parseStream(TC_NetWorkBuffer& in, vector<char>& out)
    {
        size_t len = offset + sizeof(T);

        if (in.getBufferLength() < len)
        {
            return TC_NetWorkBuffer::PACKET_LESS;
        }

        string header;
        in.getHeader(len, header);

        assert(header.size() == len);

        T iHeaderLen = 0;

        ::memcpy(&iHeaderLen, header.c_str() + offset, sizeof(T));

        if (netorder)
        {
            iHeaderLen = net2host<T>(iHeaderLen);
        }

        //长度保护一下
        if (iHeaderLen < (T)(len) || (uint32_t)iHeaderLen > TARS_NET_MAX_PACKAGE_SIZE)
        {
            return TC_NetWorkBuffer::PACKET_ERR;
        }

        if (in.getBufferLength() < (uint32_t)iHeaderLen)
        {
            return TC_NetWorkBuffer::PACKET_LESS;
        }

        in.getHeader(iHeaderLen, out);

        assert(out.size() == iHeaderLen);

        in.moveHeader(iHeaderLen);

        return TC_NetWorkBuffer::PACKET_FULL;
    }

    // template<tars::Int32 iMaxLength>
    // static int parseLen(string &in, string &out)
    // {
    //     if(in.length() < sizeof(tars::Int32))
    //     {
    //         return TC_EpollServer::PACKET_LESS;
    //     }

    //     tars::Int32 iHeaderLen;

    //     memcpy(&iHeaderLen, in.c_str(), sizeof(tars::Int32));

    //     iHeaderLen = ntohl(iHeaderLen);

    //     if(iHeaderLen < tars::Int32(sizeof(tars::Int32))|| iHeaderLen > iMaxLength)
    //     {
    //         return TC_EpollServer::PACKET_ERR;
    //     }

    //     if((int)in.length() < iHeaderLen)
    //     {
    //         return TC_EpollServer::PACKET_LESS;
    //     }

    //     out = in.substr(sizeof(tars::Int32), iHeaderLen - sizeof(tars::Int32));

    //     in  = in.substr(iHeaderLen);

    //     return TC_EpollServer::PACKET_FULL;
    // }

    // /**
    //  * 解析协议
    //  * @param in, 目前的buffer
    //  * @param out, 一个完整的包
    //  *
    //  * @return int, 0表示没有接收完全, 1表示收到一个完整包
    //  */
    // // static int parseAdmin(string &in, string &out);

    // /**
    //  *
    //  * @param T
    //  * @param offset
    //  * @param netorder
    //  * @param in
    //  * @param out
    //  * @return int
    //  */
    // template<size_t offset, typename T, bool netorder>
    // static int parseStream(string& in, string& out)
    // {
    //     if(in.length() < offset + sizeof(T))
    //     {
    //         return TC_EpollServer::PACKET_LESS;
    //     }

    //     T iHeaderLen = 0;

    //     ::memcpy(&iHeaderLen, in.c_str() + offset, sizeof(T));

    //     if (netorder)
    //     {
    //         iHeaderLen = net2host<T>(iHeaderLen);
    //     }

    //     if (iHeaderLen < (T)(offset + sizeof(T)) || (uint32_t)iHeaderLen > 100000000)
    //     {
    //         return TC_EpollServer::PACKET_ERR;
    //     }

    //     if (in.length() < (uint32_t)iHeaderLen)
    //     {
    //         return TC_EpollServer::PACKET_LESS;
    //     }

    //     out = in.substr(0, iHeaderLen);

    //     in  = in.substr(iHeaderLen);

    //     return TC_EpollServer::PACKET_FULL;
    // }
};

typedef std::function<void(const RequestPacket&, shared_ptr<TC_NetWorkBuffer::SendBuffer>&)> request_protocol;
/**
 * 接收协议处理, 返回值表示解析了多少字节
 * 框架层会自动对处理了包做处理
 */
typedef std::function<TC_NetWorkBuffer::PACKET_TYPE(TC_NetWorkBuffer&, ResponsePacket&)> response_protocol;
typedef std::function<TC_NetWorkBuffer::PACKET_TYPE(TC_NetWorkBuffer&, ResponsePacket&)> response_ex_protocol;

// using request_protocol = std::function<void (const RequestPacket& , string& )>;
// /**
//  * 接收协议处理, 返回值表示解析了多少字节
//  * 框架层会自动对处理了包做处理
//  */
// using response_protocol = std::function<size_t (const char *, size_t, list<ResponsePacket>&)>;
// using response_ex_protocol = std::function<size_t (const char *, size_t, list<ResponsePacket>&, void*)>;

//////////////////////////////////////////////////////////////////////
/**
 * 客户端自定义协议设置
 */
class ProxyProtocol
{
public:
    /**
     * 构造
     */
    ProxyProtocol() : requestFunc(streamRequest) {}

    /**
     * 普通二进制请求包
     * @param request
     * @param buff
     */
    static void streamRequest(const RequestPacket& request, shared_ptr<TC_NetWorkBuffer::SendBuffer>& buff)
    {
	    buff->setBuffer(request.sBuffer);
    }

    // /**
    //  * 普通二进制请求包
    //  * @param request
    //  * @param buff
    //  */
    // static void streamRequest(const RequestPacket& request, string& buff)
    // {
    //     buff.assign((const char*)(&request.sBuffer[0]), request.sBuffer.size());
    // }

    // /**
    //  * 普通二进制 tars 请求包
    //  * @param request
    //  * @param buff
    //  */
    // template
    // <
    //     size_t   offset,
    //     typename T,
    //     bool     netorder,
    //     size_t   idOffset,
    //     typename K,
    //     bool     idNetorder,
    //     size_t   packetMaxSize
    // >
    // static size_t streamResponse(const char* recvBuffer, size_t length, list<ResponsePacket>& done)
    // {
    //     size_t pos = 0;

    //     while (pos < length)
    //     {
    //         uint32_t len = length - pos;

    //         if (len < offset + sizeof(T))
    //         {
    //             break;
    //         }

    //         T iHeaderLen = 0;

    //         ::memcpy(&iHeaderLen, recvBuffer + pos + offset, sizeof(T));

    //         if (netorder)
    //         {
    //             iHeaderLen = net2host<T>(iHeaderLen);
    //         }

    //         //做一下保护,长度大于10M
    //         size_t sizeHeaderLen = static_cast<size_t>(iHeaderLen);
    //         if (sizeHeaderLen > packetMaxSize || iHeaderLen == 0)
    //         {
    //             throw TarsDecodeException("packet length too long or zero,len:(" + TC_Common::tostr(packetMaxSize) + ")" + TC_Common::tostr(iHeaderLen));
    //         }

    //         //包没有接收全
    //         if (len < iHeaderLen)
    //         {
    //             break;
    //         }

    //         ResponsePacket rsp;

    //         rsp.sBuffer.reserve(iHeaderLen);
    //         rsp.sBuffer.resize(iHeaderLen);

    //         ::memcpy(&rsp.sBuffer[0], recvBuffer + pos, iHeaderLen);

    //         K requestId;

    //         ::memcpy(&requestId, recvBuffer + pos + idOffset, sizeof(K));

    //         if (idNetorder)
    //         {
    //             requestId = net2host<K>(requestId);
    //         }

    //         rsp.iRequestId = static_cast<uint32_t>(requestId);

    //         done.push_back(rsp);

    //         pos += iHeaderLen;
    //     }

    //     return pos;
    // }

    // template
    // <
    //     size_t   offset,
    //     typename T,
    //     bool     netorder,
    //     size_t   idOffset,
    //     typename K,
    //     bool     idNetorder
    // >
    // static size_t streamResponse(const char* recvBuffer, size_t length, list<ResponsePacket>& done)
    // {
    //     return streamResponse<offset, T, netorder, idOffset, K, idNetorder, 10000000>(recvBuffer, length, done);
    // }


    /**
     * 普通二进制包普taf请求包
     * @param request
     * @param buff
     */
    template
        <
            size_t offset,
            typename T,
            bool netorder,
            size_t idOffset,
            typename K,
            bool idNetorder,
            size_t packetMaxSize
        >
    static size_t streamResponse(TC_NetWorkBuffer &in, ResponsePacket& rsp)
    {
        size_t len = offset + sizeof(T);

        if (in.getBufferLength() < len)
        {
            return TC_NetWorkBuffer::PACKET_LESS;
        }

        string header;
        in.getHeader(len, header);

        assert(header.size() == len);

        T iHeaderLen = 0;

        ::memcpy(&iHeaderLen, header.c_str() + offset, sizeof(T));

        if (netorder)
        {
            iHeaderLen = net2host<T>(iHeaderLen);
        }

        //做一下保护
        size_t sizeHeaderLen = static_cast<size_t>(iHeaderLen);
        if (sizeHeaderLen > packetMaxSize || iHeaderLen == 0)
        {
            throw TarsDecodeException("packet length too long or zero,len:(" + TC_Common::tostr(packetMaxSize) + ")" + TC_Common::tostr(iHeaderLen));
        }

        //包没有接收全
        if (in.getBufferLength() < (uint32_t)iHeaderLen)
        {
            return TC_NetWorkBuffer::PACKET_LESS;
        }

        //把buffer读取到
        in.getHeader(iHeaderLen, rsp.sBuffer);

        K requestId;

        vector<char> tmp;
        in.moveHeader(idOffset);
        in.getHeader(sizeof(K), tmp);

        if (idNetorder)
        {
            requestId = net2host<K>(*(K*)tmp.data());
        }

        rsp.iRequestId = static_cast<uint32_t>(requestId);

        return TC_NetWorkBuffer::PACKET_FULL;
    }

    template
        <
            size_t offset,
            typename T,
            bool netorder,
            size_t idOffset,
            typename K,
            bool idNetorder
        >
    static size_t streamResponse(TC_NetWorkBuffer &in, ResponsePacket &done)
    {
        return streamResponse<offset, T, netorder, idOffset, K, idNetorder, TARS_NET_MAX_PACKAGE_SIZE>(in, done);
    }

    /**
     * tup响应包(tup的响应会放在ResponsePacket的buffer中)
     * @param request
     * @param buff
     */

    /**
     * wup响应包(wup的响应会放在ResponsePacket的buffer中)
     * @param request
     * @param buff
     */
    static TC_NetWorkBuffer::PACKET_TYPE tupResponse(TC_NetWorkBuffer &in, ResponsePacket& done)
    {
        return tupResponseLen<TARS_NET_MIN_PACKAGE_SIZE, TARS_NET_MAX_PACKAGE_SIZE>(in, done);
    }

    // static size_t tupResponse(const char* recvBuffer, size_t length, list<ResponsePacket>& done)
    // {
    //     return tupResponseLen<10000000>(recvBuffer,length,done);
    // }


    template<uint32_t iMinLength, uint32_t iMaxLength>
    static TC_NetWorkBuffer::PACKET_TYPE tupResponseLen(TC_NetWorkBuffer &in, ResponsePacket& rsp)
    {
        uint32_t len = (uint32_t)in.getBufferLength();

        //收到的字节数太少, 还需要继续接收
        if(len < sizeof(uint32_t))
            return TC_NetWorkBuffer::PACKET_LESS;

        //获取包总体长度
        uint32_t iHeaderLen = in.getValueOf4();

        //做一下保护,长度大于10M
        if (iHeaderLen < iMinLength || iHeaderLen > iMaxLength)
        {
            throw TarsDecodeException("packet length too long or too short,len:" + TC_Common::tostr(iHeaderLen));
        }

            //包没有接收全
        if (len < iHeaderLen)
        {
            //看看包头是否正确
            static const uint32_t head = 20;

            if (len >= head)
            {
                string buffer;
                in.getHeader(head, buffer);

                TarsInputStream<BufferReader> is;

                is.setBuffer(buffer.c_str() + sizeof(tars::Int32), head);

                // is.setBuffer(recvBuffer + pos + sizeof(tars::Int32), head);

                //tup回来是requestpackage
                RequestPacket rsp;

                is.read(rsp.iVersion, 1, true);

                if (rsp.iVersion != TUPVERSION)
                {
                    throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(rsp.iVersion));
                }

                is.read(rsp.cPacketType, 2, true);

                if (rsp.cPacketType != TARSNORMAL)
                {
                    throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)rsp.cPacketType));
                }

                is.read(rsp.iMessageType, 3, true);
                is.read(rsp.iRequestId, 4, true);
            }
            return TC_NetWorkBuffer::PACKET_LESS;
        }
        else
        {
            //buffer包括4个字节长度
            vector<char> buffer;
            buffer.resize(iHeaderLen);

            in.getHeader(iHeaderLen, buffer);
            TarsInputStream<BufferReader> is;

            is.setBuffer(buffer.data() + sizeof(tars::Int32), buffer.size() - sizeof(tars::Int32));

            // is.setBuffer(&(buffer[0]) + sizeof(tars::Int32), buffer.size() - sizeof(tars::Int32));

            // pos += iHeaderLen;

            //TUP的响应包其实也是返回包
            RequestPacket req;
            req.readFrom(is);

            if (req.iVersion != TUPVERSION )
            {
                throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(req.iVersion));
            }

            if (req.cPacketType != TARSNORMAL)
            {
                throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)req.cPacketType));
            }

            ResponsePacket rsp;
            rsp.cPacketType     = req.cPacketType;
            rsp.iMessageType    = req.iMessageType;
            rsp.iRequestId      = req.iRequestId;
            rsp.iVersion        = req.iVersion;
            rsp.context         = req.context;
            //tup的响应包直接放入到sBuffer里面
            rsp.sBuffer         = buffer;

            in.moveHeader(iHeaderLen);
        }

        return TC_NetWorkBuffer::PACKET_FULL;
    }

    // template<uint32_t iMaxLength>
    // static size_t tupResponseLen(const char* recvBuffer, size_t length, list<ResponsePacket>& done)
    // {
    //     size_t pos = 0;
    //     while (pos < length)
    //     {
    //         uint32_t len = length - pos;

    //         if(len < sizeof(tars::Int32))
    //         {
    //             break;
    //         }

    //         uint32_t iHeaderLen = ntohl(*(uint32_t*)(recvBuffer + pos));

    //         //做一下保护,长度大于10M
    //         if (iHeaderLen > iMaxLength || iHeaderLen < sizeof(tars::Int32))
    //         {
    //             throw TarsDecodeException("packet length too long or too short,len:" + TC_Common::tostr(iHeaderLen));
    //         }

    //         //包没有接收全
    //         if (len < iHeaderLen)
    //         {
    //             //看看包头是否正确
    //             static const uint32_t head = 20;

    //             if(len >= head)
    //             {
    //                 TarsInputStream<BufferReader> is;

    //                 is.setBuffer(recvBuffer + pos + sizeof(tars::Int32), head);

    //                 //tup回来是requestpackage
    //                 RequestPacket rsp;

    //                 is.read(rsp.iVersion, 1, true);

    //                 if (rsp.iVersion != TUPVERSION)
    //                 {
    //                     throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(rsp.iVersion));
    //                 }

    //                 is.read(rsp.cPacketType, 2, true);

    //                 if (rsp.cPacketType != TARSNORMAL)
    //                 {
    //                     throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)rsp.cPacketType));
    //                 }

    //                 is.read(rsp.iMessageType, 3, true);
    //                 is.read(rsp.iRequestId, 4, true);
    //             }
    //             break;
    //         }
    //         else
    //         {
    //             TarsInputStream<BufferReader> is;

    //             //buffer包括4个字节长度
    //             vector<char> buffer;

    //             buffer.reserve(iHeaderLen);
    //             buffer.resize(iHeaderLen);
    //             memcpy(&(buffer[0]), recvBuffer + pos, iHeaderLen);

    //             is.setBuffer(&(buffer[0]) + sizeof(tars::Int32), buffer.size() - sizeof(tars::Int32));

    //             pos += iHeaderLen;

    //             //TUP的响应包其实也是返回包
    //             RequestPacket req;
    //             req.readFrom(is);

    //             if (req.iVersion != TUPVERSION )
    //             {
    //                 throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(req.iVersion));
    //             }

    //             if (req.cPacketType != TARSNORMAL)
    //             {
    //                 throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)req.cPacketType));
    //             }

    //             ResponsePacket rsp;
    //             rsp.cPacketType     = req.cPacketType;
    //             rsp.iMessageType    = req.iMessageType;
    //             rsp.iRequestId      = req.iRequestId;
    //             rsp.iVersion        = req.iVersion;
    //             rsp.context         = req.context;
    //             //tup的响应包直接放入到sBuffer里面
    //             rsp.sBuffer         = buffer;

    //             done.push_back(rsp);
    //         }
    //     }

    //     return pos;
    // }


public:
    /**
     * tars请求包
     * @param request
     * @param buff
     */
    static void tarsRequest(const RequestPacket& request, shared_ptr<TC_NetWorkBuffer::SendBuffer>& buff);
    // static void tarsRequest(const RequestPacket& request, string& buff);

    /**
     * tars响应包解析
     * @param recvBuffer
     * @param done
     */
    static TC_NetWorkBuffer::PACKET_TYPE tarsResponse(TC_NetWorkBuffer &in, ResponsePacket& done)
    {
        return tarsResponseLen<TARS_NET_MIN_PACKAGE_SIZE, TARS_NET_MAX_PACKAGE_SIZE>(in, done);
    }
    // static size_t tarsResponse(const char* recvBuffer, size_t length, list<ResponsePacket>& done)
    // {
    //     return tarsResponseLen<10000000>(recvBuffer,length,done);
    // }


    template<uint32_t iMinLength, uint32_t iMaxLength>
    static TC_NetWorkBuffer::PACKET_TYPE tarsResponseLen(TC_NetWorkBuffer &in, ResponsePacket& rsp)
    {
        uint32_t len = (uint32_t)in.getBufferLength();

        //收到的字节数太少, 还需要继续接收
        if(len < sizeof(uint32_t))
            return TC_NetWorkBuffer::PACKET_LESS;

        //获取包总体长度
        uint32_t iHeaderLen = in.getValueOf4();

        //做一下保护,长度大于10M
        if (iHeaderLen < iMinLength || iHeaderLen > iMaxLength)
        {
            throw TarsDecodeException("packet length too long or too short,len:" + TC_Common::tostr(iHeaderLen));
        }

        //包没有接收全
        if (len < iHeaderLen)
        {
            //看看包头是否正确
            static const uint32_t head = 20;

            if (len >= head)
            {
                string buffer;
                in.getHeader(head, buffer);

                TarsInputStream<BufferReader> is;
                is.setBuffer(buffer.c_str() + sizeof(tars::Int32), head);

                ResponsePacket rsp;
                is.read(rsp.iVersion, 1, false);

                if (rsp.iVersion != TARSVERSION)
                {
                    throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(rsp.iVersion));
                }

                is.read(rsp.cPacketType, 2, false);

                if (rsp.cPacketType != TARSNORMAL)
                {
                    throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)rsp.cPacketType));
                }

                is.read(rsp.iRequestId, 3, false);
                is.read(rsp.iMessageType, 4, false);
                is.read(rsp.iRet, 5, false);

                if (rsp.iRet < TARSSERVERUNKNOWNERR)
                {
                    throw TarsDecodeException("response value not correct, value:" + TC_Common::tostr(rsp.iRet));
                }
            }

            return TC_NetWorkBuffer::PACKET_LESS;
        }
        else
        {
	        vector<char> buffer;
            bool ret = in.parseBufferOf4(buffer, iMinLength, iMaxLength);
            if(!ret)
            {
	            throw TarsDecodeException("parse buffer exception");
            }

            TarsInputStream<BufferReader> is;
            is.setBuffer(buffer.data(), buffer.size());

            // pos += iHeaderLen;

            ResponsePacket rsp;
            rsp.readFrom(is);

            if (rsp.iVersion != TARSVERSION)
            {
                throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(rsp.iVersion));
            }

            if (rsp.cPacketType != TARSNORMAL)
            {
                throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)rsp.cPacketType));
            }

            if (rsp.iRet < TARSSERVERUNKNOWNERR)
            {
                throw TarsDecodeException("response value not correct, value:" + TC_Common::tostr(rsp.iRet));
            }
        }

        return TC_NetWorkBuffer::PACKET_FULL;
    }

    // template<uint32_t iMaxLength>
    // static size_t tarsResponseLen(const char* recvBuffer, size_t length, list<ResponsePacket>& done)
    // {
    //     size_t pos = 0;
    //     while (pos < length)
    //     {
    //         uint32_t len = length - pos;
    //         if(len < sizeof(tars::Int32))
    //         {
    //             break;
    //         }

    //         uint32_t iHeaderLen = ntohl(*(uint32_t*)(recvBuffer + pos));

    //         //做一下保护,长度大于10M
    //         if (iHeaderLen > iMaxLength || iHeaderLen < sizeof(tars::Int32))
    //         {
    //             throw TarsDecodeException("packet length too long or too short,len:" + TC_Common::tostr(iHeaderLen));
    //         }

    //         //包没有接收全
    //         if (len < iHeaderLen)
    //         {
    //             //看看包头是否正确
    //             static const uint32_t head = 20;

    //             if(len >= head)
    //             {
    //                 TarsInputStream<BufferReader> is;
    //                 is.setBuffer(recvBuffer + pos + sizeof(tars::Int32), head);

    //                 ResponsePacket rsp;
    //                 is.read(rsp.iVersion, 1, false);

    //                 if (rsp.iVersion != TARSVERSION)
    //                 {
    //                     throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(rsp.iVersion));
    //                 }

    //                 is.read(rsp.cPacketType, 2, false);

    //                 if (rsp.cPacketType != TARSNORMAL)
    //                 {
    //                     throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)rsp.cPacketType));
    //                 }

    //                 is.read(rsp.iRequestId, 3, false);
    //                 is.read(rsp.iMessageType, 4, false);
    //                 is.read(rsp.iRet, 5, false);

    //                 if (rsp.iRet < TARSSERVERUNKNOWNERR)
    //                 {
    //                     throw TarsDecodeException("response value not correct, value:" + TC_Common::tostr(rsp.iRet));
    //                 }
    //             }
    //             break;
    //         }
    //         else
    //         {
    //             TarsInputStream<BufferReader> is;
    //             is.setBuffer(recvBuffer + pos + sizeof(tars::Int32), iHeaderLen - sizeof(tars::Int32));
    //             pos += iHeaderLen;

    //             ResponsePacket rsp;
    //             rsp.readFrom(is);

    //             if (rsp.iVersion != TARSVERSION)
    //             {
    //                 throw TarsDecodeException("version not correct, version:" + TC_Common::tostr(rsp.iVersion));
    //             }

    //             if (rsp.cPacketType != TARSNORMAL)
    //             {
    //                 throw TarsDecodeException("packettype not correct, packettype:" + TC_Common::tostr((int)rsp.cPacketType));
    //             }

    //             if (rsp.iRet < TARSSERVERUNKNOWNERR)
    //             {
    //                 throw TarsDecodeException("response value not correct, value:" + TC_Common::tostr(rsp.iRet));
    //             }

    //             done.push_back(rsp);
    //         }
    //     }

    //     return pos;
    // }

public:
    request_protocol  requestFunc;

    response_protocol responseFunc;

    response_ex_protocol responseExFunc;
};

class TC_NgHttp2;

void http1Request(const tars::RequestPacket& request, std::string& out);
size_t http1Response(const char* recvBuffer, size_t length, std::list<tars::ResponsePacket>& done);
std::string encodeHttp2(RequestPacket& request, TC_NgHttp2* session);

// ENCODE function, called by network thread
void http2Request(const tars::RequestPacket& request, std::string& out);

// DECODE function, called by network thread
size_t http2Response(const char* recvBuffer, size_t length, std::list<tars::ResponsePacket>& done, void* userData);


//////////////////////////////////////////////////////////////////////
}
#endif
