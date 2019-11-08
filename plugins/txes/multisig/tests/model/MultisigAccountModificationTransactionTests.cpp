/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/model/MultisigAccountModificationTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/MultisigTestUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/Alignment.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	using TransactionType = MultisigAccountModificationTransaction;

#define TEST_CLASS MultisigAccountModificationTransactionTests

	// region size + alignment + properties

#define TRANSACTION_FIELDS FIELD(MinRemovalDelta) FIELD(MinApprovalDelta) FIELD(PublicKeyAdditionsCount) FIELD(PublicKeyDeletionsCount)

	namespace {
		template<typename T>
		void AssertTransactionHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize + sizeof(uint32_t);

#define FIELD(X) expectedSize += sizeof(T::X);
			TRANSACTION_FIELDS
#undef FIELD

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 4 + 4u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasProperAlignment() {
#define FIELD(X) EXPECT_ALIGNED(T, X);
			TRANSACTION_FIELDS
#undef FIELD

			EXPECT_EQ(0u, sizeof(T) % 8);
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_Multisig_Account_Modification, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

#undef TRANSACTION_FIELDS

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(MultisigAccountModification)

	// endregion

	// region data pointers

	namespace {
		struct MultisigAccountModificationTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t numAdditions, uint8_t numDeletions) {
				uint32_t entitySize = sizeof(TransactionType) + (numAdditions + numDeletions) * Key::Size;
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->PublicKeyAdditionsCount = numAdditions;
				pTransaction->PublicKeyDeletionsCount = numDeletions;
				return pTransaction;
			}

			static constexpr size_t GetAttachment1Size(uint8_t numAdditions) {
				return numAdditions * Key::Size;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.PublicKeyAdditionsPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.PublicKeyDeletionsPtr();
			}
		};
	}

	DEFINE_DUAL_ATTACHMENT_POINTER_TESTS(TEST_CLASS, MultisigAccountModificationTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.PublicKeyAdditionsCount = 7;
		transaction.PublicKeyDeletionsCount = 4;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 11 * Key::Size, realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.PublicKeyAdditionsCount);
		test::SetMaxValue(transaction.PublicKeyDeletionsCount);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + (0xFF + 0xFF) * Key::Size, realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion

	// region ExtractAdditionalRequiredCosignatories

	TEST(TEST_CLASS, ExtractAdditionalRequiredCosignatories_NoModifications) {
		// Arrange:
		auto pTransaction = test::CreateMultisigAccountModificationTransaction(test::GenerateRandomByteArray<Key>(), 0, 0);

		// Act:
		auto additionalCosignatories = ExtractAdditionalRequiredCosignatories(*pTransaction);

		// Assert:
		EXPECT_EQ(utils::KeySet(), additionalCosignatories);
	}

	TEST(TEST_CLASS, ExtractAdditionalRequiredCosignatories_AddModifications) {
		// Arrange:
		auto pTransaction = test::CreateMultisigAccountModificationTransaction(test::GenerateRandomByteArray<Key>(), 2, 0);

		// Act:
		auto additionalCosignatories = ExtractAdditionalRequiredCosignatories(*pTransaction);

		// Assert:
		const auto* pPublicKeyAdditions = pTransaction->PublicKeyAdditionsPtr();
		EXPECT_EQ(utils::KeySet({ pPublicKeyAdditions[0], pPublicKeyAdditions[1] }), additionalCosignatories);
	}

	TEST(TEST_CLASS, ExtractAdditionalRequiredCosignatories_DelModifications) {
		// Arrange:
		auto pTransaction = test::CreateMultisigAccountModificationTransaction(test::GenerateRandomByteArray<Key>(), 0, 2);

		// Act:
		auto additionalCosignatories = ExtractAdditionalRequiredCosignatories(*pTransaction);

		// Assert:
		EXPECT_EQ(utils::KeySet(), additionalCosignatories);
	}

	TEST(TEST_CLASS, ExtractAdditionalRequiredCosignatories_AddAndDelModifications) {
		// Arrange:
		auto pTransaction = test::CreateMultisigAccountModificationTransaction(test::GenerateRandomByteArray<Key>(), 2, 2);

		// Act:
		auto additionalCosignatories = ExtractAdditionalRequiredCosignatories(*pTransaction);

		// Assert:
		const auto* pPublicKeyAdditions = pTransaction->PublicKeyAdditionsPtr();
		EXPECT_EQ(utils::KeySet({ pPublicKeyAdditions[0], pPublicKeyAdditions[1] }), additionalCosignatories);
	}

	// endregion
}}
