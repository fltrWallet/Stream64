//===----------------------------------------------------------------------===//
//
// This source file is part of the Stream64 open source project
//
// Copyright (c) 2022 fltrWallet AG and the Stream64 project authors
// Licensed under Apache License v2.0
//
// See LICENSE.md for license information
//
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
import CStream64
import HaByLo

public struct Stream64: Codable, Sequence {
    @usableFromInline
    let data: [UInt8]
    @usableFromInline
    let count: Int
    @usableFromInline
    let p: Int

    @inlinable
    public init(data: [UInt8], count: Int, p: Int) {
        self.data = data
        self.count = count
        self.p = p
    }
    
    @inlinable
    public init(data: [UInt8], p: Int) {
        precondition(p > 7)
        self.count = data.count * 8 / p
        self.data = data + (0..<7).map { _ in UInt8(0) }
        self.p = p
    }
    
    @inlinable
    public func makeIterator() -> StreamIterator {
        StreamIterator(self)
    }
}

public struct StreamIterator: IteratorProtocol {
    @usableFromInline
    let stream64: Stream64
    @usableFromInline
    var state: CStream64.gcs_state
    @usableFromInline
    var index: Int = 0
    
    @inlinable
    public init(_ stream64: Stream64) {
        self.stream64 = stream64
        self.state = CStream64.initialize()
    }
    
    @inlinable
    public mutating func next() -> UInt64? {
        precondition(
            state.bufpos < stream64.data.count - 1
                || state.bitpos < 64 - stream64.p
        )
        
        guard self.index < self.stream64.count
        else {
            return nil
        }
        
        defer { self.index += 1 }
        
        return self.stream64.data.withUnsafeBufferPointer {
            read_p($0.baseAddress!,
                   self.stream64.p,
                   &self.state)
        }
    }
}

public enum Stream64Error: Error {
    case illegalInput
}

public extension Stream64 {
    @inlinable
    static func streamWrite(values: [UInt64], p: Int) throws -> [UInt8] {
        let bitsCount = values.count * p
        let bytesCountCeil = (bitsCount + 7) / 8
        
        return try Array(unsafeUninitializedCapacity: bytesCountCeil) { buffer, setSizeTo in
            let cRet = values.withUnsafeBufferPointer { values in
                write_bitstream(values.baseAddress!, values.count, p, buffer.baseAddress, &setSizeTo)
            }
            guard cRet == 0
            else { throw Stream64Error.illegalInput }
        }
    }
}
