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

public struct GolombCodedSetClient {
    public init(decode: @escaping ([UInt8], Int, Int) -> [UInt64]?,
                encode: @escaping ([UInt64], Int) -> [UInt8]?) {
        self.decode0 = decode
        self.encode0 = encode
    }
    
    public func decode(compressed: [UInt8], n: Int, p: Int) -> [UInt64]? {
        guard n > 0
        else { return [] }
        
        return self.decode0(compressed, n, p)
    }
    let decode0: ([UInt8], Int, Int) -> [UInt64]?
    
    public func encode(sorted: [UInt64], p: Int) -> [UInt8]? {
        guard sorted.count > 0
        else { return [] }
        
        return self.encode0(sorted, p)
    }
    let encode0: ([UInt64], Int) -> [UInt8]?
    
    public func encode(unsorted: [UInt64], p: Int) -> [UInt8]? {
        self.encode(sorted: unsorted.sorted(), p: p)
    }
}

fileprivate struct Stream64GCS {
    init() {}

    func decode(compressed: [UInt8], n: Int, p: Int) -> [UInt64]? {
        var state: CStream64.gcs_state = CStream64.initialize()
        var accumulator: UInt64 = 0
        let result: [UInt64] = .init(unsafeUninitializedCapacity: n) { result, setSizeTo in
            compressed.withUnsafeBufferPointer { compressed in
                for i in (0..<n) {
                    guard state.bufpos < compressed.count - 1
                          || state.bitpos < 64 - p
                    else {
                        setSizeTo = 0
                        return
                    }
                    
                    accumulator += read_gcs_next(compressed.baseAddress!,
                                                 p,
                                                 &state)
                    result[i] = accumulator
                }
                setSizeTo = n
            }
        }
        
        guard result.count > 0
        else { return nil }
        
        return result
    }
    
    func encode(sorted: [UInt64], p: Int) -> [UInt8]? {
        let result: [UInt8] = Array(unsafeUninitializedCapacity: sorted.count * 8) { buffer, setSizeTo in
            let cRet = sorted.withUnsafeBufferPointer { data in
                write_gcs(p, data.baseAddress!, data.count, buffer.baseAddress, &setSizeTo)
            }
            guard cRet == 0
            else {
                setSizeTo = 0
                return
            }
        }
        
        guard result.count > 0
        else { return nil }
        
        return result
    }
}



public extension GolombCodedSetClient {
    static var stream64: Self = {
        let stream64 = Stream64GCS()
        
        return Self.init(decode: stream64.decode(compressed:n:p:),
                         encode: stream64.encode(sorted:p:))
    }()
}
